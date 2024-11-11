#include <pdcurses/curses.h>
#include <windows.h>
#include <sys/stat.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

#define LARGURA_MAX_JANELA GetSystemMetrics(SM_CXMAXIMIZED)
#define ALTURA_MAX_JANELA GetSystemMetrics(SM_CYMAXIMIZED)
// #define LARGURA_MAX_JANELA 1300
// #define ALTURA_MAX_JANELA 760
#define LARGURA_MAX_CONSOLE getmaxx(stdscr)
#define ALTURA_MAX_CONSOLE getmaxy(stdscr)
#define VERSAO_PROGRAMA 0.1

typedef struct {

    unsigned short int id;
    char id_fabrica[16];
    char nome[51];
    char fabricante[15];
    char unidade[3];
    unsigned short int quantidade;
    float valor_uni;
    float subtotal;

} produto;

char buffer_sql[120];
char *erro_sql;

int menu(int opc);
int sql_retorno(void *Inutilizado, int argc, char **argv, char **coluna);
int select_id(sqlite3 *BD, int id, produto *P);
int adicionar_produto(sqlite3 *BD, produto P);

int main()
{
//  TELA DE ERRO PARA RESOLUÇÃO DO MONITOR FOR PEQUENA DEMAIS          

    if (LARGURA_MAX_JANELA < 1150 || ALTURA_MAX_JANELA < 680)
    {
        SetWindowPos(GetConsoleWindow(), HWND_NOTOPMOST, ALTURA_MAX_JANELA/2, LARGURA_MAX_JANELA/2, ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, SWP_SHOWWINDOW);
        initscr();
        noecho();
        cbreak();
        curs_set(0);

        WINDOW * janela_fim = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
        box(janela_fim, 0, 0);        
        mvwprintw(janela_fim, (ALTURA_MAX_CONSOLE/2)-1, (LARGURA_MAX_CONSOLE/2)-37, "A resolução do seu Monitor não suporta a visualização do programa =(");
        refresh();
        wrefresh(janela_fim);
        getch();

        delwin(janela_fim);
        endwin();
        return 1;
    }
    else SetWindowPos(GetConsoleWindow(), HWND_NOTOPMOST, 0, 0, LARGURA_MAX_JANELA, ALTURA_MAX_JANELA, SWP_SHOWWINDOW);

//  CRIAÇÃO DO BANCO DE DADOS E DAS CONFIGURAÇÕES

    char nome_empresa[51];
    int total_itens;
    
    mkdir("Dados_Programa");
    FILE *configs = fopen("Dados_Programa\\configs.txt", "r");

    sqlite3 *banco_dados;
    sqlite3_open("Dados_Programa\\teste.db", &banco_dados);
    const char criar_tabela[] = "CREATE TABLE IF NOT EXISTS produtos ( id INTEGER PRIMARY KEY AUTOINCREMENT, id_fabrica UNSIGNED BIG INT UNIQUE NOT NULL, nome VARCHAR(30), fabricante VARCHAR(14), unidade CHAR(2), quantidade TINYINT, valor_uni DECIMAL(5,2), subtotal DECIMAL(5,2) );";
    sqlite3_exec(banco_dados, criar_tabela, sql_retorno, 0, &erro_sql);

    if (configs == NULL)
    {
        fclose(configs);
        configs = fopen("Dados_Programa\\configs.txt", "w+");
        
        initscr();
        
            WINDOW * janela_nome_empresa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
            WINDOW * inserir_nome_empresa = newwin(3, 52, ( ALTURA_MAX_CONSOLE - 1 ) / 2, ( LARGURA_MAX_CONSOLE - 52 ) / 2);
            refresh();

            box(janela_nome_empresa, 0, 0);
            wborder(inserir_nome_empresa, 9553, 9553, 9552, 9552, 9556, 9559, 9562, 9565);

            char buffer_empresa[51];

                mvwprintw(janela_nome_empresa, ( ( ALTURA_MAX_CONSOLE - 1 ) / 2 ) - 3, ( ( LARGURA_MAX_CONSOLE - 52 ) / 2 ) + 7, "Por favor insira o nome da sua Empresa");
                wrefresh(janela_nome_empresa);
                wrefresh(inserir_nome_empresa);
                
                mvwgetstr(inserir_nome_empresa, 1, 1, buffer_empresa);
                strcpy(nome_empresa, buffer_empresa);

                sqlite3_exec(banco_dados, "SELECT COUNT(id) FROM produtos;", sql_retorno, 0, NULL);
                total_itens = atoi(buffer_sql);
                fprintf(configs, buffer_empresa);
                fprintf(configs, ",%d",total_itens);
            
            delwin(inserir_nome_empresa);
            delwin(janela_nome_empresa);

        endwin();
    }
    else fscanf(configs, "%50[^,],%d",nome_empresa,&total_itens);
    
    fclose(configs);

//  MENU PRINCIPAL

    int rodar_programa = 1;
    do {
    time_t tempo_atual = time(NULL);
    struct tm *data_atual = localtime(&tempo_atual);

    initscr();
    noecho();
    cbreak();
    curs_set(0);

    const int opcaoMaiorY = ALTURA_MAX_CONSOLE;
    const int opcaoMaiorX = LARGURA_MAX_CONSOLE/3;
    const int opcaoMenorY = ALTURA_MAX_CONSOLE-16;
    const int InfoX = LARGURA_MAX_CONSOLE-(opcaoMaiorX);
    const int tabelaX = ((17*InfoX)/20);

    WINDOW * caixa_opcoes_maior = newwin( opcaoMaiorY, opcaoMaiorX, 0, 0 );
    WINDOW * caixa_opcoes_menor = newwin( opcaoMenorY, opcaoMaiorX, 10, 0 );
    WINDOW * caixaInfo = newwin( opcaoMaiorY, InfoX, 0, opcaoMaiorX - 1 );
    WINDOW * tabela_produtos = newwin( opcaoMenorY, tabelaX, 10, opcaoMaiorX + ((InfoX - tabelaX)/2));
    refresh();

        box(tabela_produtos, 0, 0);
        box(caixa_opcoes_maior, 0, 0);
        wborder(caixaInfo, 0, 0, 0, 0, 9516, 0, 9524, 0);
        wborder(caixa_opcoes_menor, 0, 0, 0, 0, 9500, 9508, 9500, 9508);

        mvwprintw(caixa_opcoes_maior, 4, ( opcaoMaiorX - 21 ) / 2,
        "CONTROLE DE ESTOQUE");

        mvwprintw(caixa_opcoes_maior, 6, ( opcaoMaiorX - 4 ) / 2, 
        "%.1f", VERSAO_PROGRAMA);

        mvwprintw(caixa_opcoes_maior, ( opcaoMaiorY - 4 ), ( opcaoMaiorX - 30 ) / 2, 
        "[  DATA: %2d  /  %2d  /  %4d  ]",
        data_atual->tm_mday, data_atual->tm_mon+1, data_atual->tm_year+1900);

        mvwprintw(caixaInfo, 5, ( InfoX - strlen(nome_empresa) ) / 2, 
        "%s", nome_empresa);

        mvwprintw(tabela_produtos, 2, 0, "├");
        for (int i = 1; i < getmaxx(tabela_produtos)-1; i++)
            mvwprintw(tabela_produtos, 2, i, "─");
        mvwprintw(tabela_produtos, 2, getmaxx(tabela_produtos)-1, "┤");

        if (tabelaX >= 104)
        {
            produto P;
            mvwprintw(tabela_produtos, 1, 1+(tabelaX - 124)/2, " ID │   COD INTERNO   │                        NOME                        │   FABRICANTE   │ UN │ QNTD │ VALOR UN │ SUBTOTAL "); 
            for (int i = 1; i <= total_itens; i++)
            {
                select_id(banco_dados, i, &P);
                mvwprintw(tabela_produtos, 2+i, 1+(tabelaX - 124)/2," %2hu │ %15s │ %50s │ %14s │ %2s │ %4hu │ %8.2f │ %8.2f ",
                P.id,P.id_fabrica,P.nome,P.fabricante,P.unidade,P.quantidade,P.valor_uni,P.subtotal);
            }
            for (int i = total_itens+3; i < opcaoMenorY-1; i++)
                mvwprintw(tabela_produtos, i,1+(tabelaX - 124)/2, "    │                 │                                                    │                │    │      │          │          ",i);
        } 
        else if (tabelaX < 124)
        {
            produto P;
            mvwprintw(tabela_produtos, 1, 1+(tabelaX - 83)/2, " ID │           NOME              │   FABRICANTE   │ QNTD │ VALOR UN │ SUBTOTAL "); 
            for (int i = 1; i <= total_itens; i++)
            {
                select_id(banco_dados, i, &P);
                mvwprintw(tabela_produtos, 2+i, 1+(tabelaX - 83)/2," %2hu │ %27s │ %14s │ %4hu │ %8.2f │ %8.2f ",
                P.id,P.nome,P.fabricante,P.quantidade,P.valor_uni,P.subtotal);
            }
            for (int i = total_itens+3; i < opcaoMenorY-1; i++)
                mvwprintw(tabela_produtos, i, 1+(tabelaX - 83)/2, "    │                             │                │      │          │          "); 
        }

    wrefresh(caixa_opcoes_maior);
    wrefresh(caixaInfo);
    wrefresh(caixa_opcoes_menor);
    wrefresh(tabela_produtos);

//  OPÇÕES DO MENU PRINCIPAL

    int opcao_selecionada = 0, tecla_pressionada;

    char opc_principal[6][30] = {
        {"Adicionar Produto"},
        {"Editar Produto"},
        {"Remover Produto"},
        {"Mostrar todos os Produtos"},
        {"Mostrar com Filtros"},
        {"Sair"}
    };
    const int qntd_opcoes = (int)(sizeof(opc_principal)/sizeof(opc_principal[0]));

    wrefresh(caixaInfo);
    keypad(caixa_opcoes_menor, true);
    do
    {
        for (int i = 0; i < qntd_opcoes; i++)
        {
            if (i == opcao_selecionada) wattron(caixa_opcoes_menor, A_STANDOUT);
            mvwprintw( caixa_opcoes_menor, (getmaxy(caixa_opcoes_menor) / 7 ) + ( i*2 ), getmaxx(caixa_opcoes_menor) / 6, "%s", opc_principal[i] );
            wattroff(caixa_opcoes_menor, A_STANDOUT);
        }

        tecla_pressionada = wgetch(caixa_opcoes_menor);

        switch(tecla_pressionada)
        {
            case KEY_UP:
                opcao_selecionada--;
                if (opcao_selecionada < 0) opcao_selecionada = qntd_opcoes-1;
                break;
            case KEY_DOWN:
                opcao_selecionada++;
                if (opcao_selecionada > qntd_opcoes-1) opcao_selecionada = 0;
                break;
        }
    } while (tecla_pressionada != 10);

    delwin(caixa_opcoes_maior);
    delwin(caixa_opcoes_menor);
    delwin(caixaInfo);
    delwin(tabela_produtos);
    endwin();

    if (opcao_selecionada == qntd_opcoes-1)
        rodar_programa = 0;
    else if (opcao_selecionada < qntd_opcoes)
        menu(opcao_selecionada+1);

    } while (rodar_programa);

    sqlite3_close(banco_dados);
	return 0;
} 

int menu(int opc)
{
    switch(opc)
    {
        case 1:
            produto P;
            char buffer_produto[51];
            int produto_valido = 1;

            initscr();
            WINDOW * janela_add_produto = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
            WINDOW * inserir_dados_produto = newwin(3, 52, ALTURA_MAX_CONSOLE/6, LARGURA_MAX_CONSOLE/8+18);
            do {
            refresh();
            box(janela_add_produto, 0, 0);

            mvwprintw(janela_add_produto, ALTURA_MAX_CONSOLE/6, LARGURA_MAX_CONSOLE/8, "ID do Fabricante: ");
            wrefresh(inserir_dados_produto);
            wrefresh(janela_add_produto);

                wgetnstr(inserir_dados_produto, buffer_produto, 15);
                strcpy(P.id_fabrica, buffer_produto);

            mvwprintw(janela_add_produto, getcury(janela_add_produto)+1, LARGURA_MAX_CONSOLE/8, "Nome do Produto: ");
            wrefresh(inserir_dados_produto);
            wrefresh(janela_add_produto);

                wgetnstr(janela_add_produto, buffer_produto, 51);
                for (int i = 0; i < 51; i++)
                    if (islower(buffer_produto[i]))
                        buffer_produto[i] = toupper(buffer_produto[i]);
                strcpy(P.nome, buffer_produto);

            mvwprintw(janela_add_produto, getcury(janela_add_produto)+1, LARGURA_MAX_CONSOLE/8, "Fabricante: ");
            wrefresh(inserir_dados_produto);
            wrefresh(janela_add_produto);

                wgetnstr(janela_add_produto, buffer_produto, 15);
                for (int i = 0; i < 15; i++)
                    if (islower(buffer_produto[i]))
                        buffer_produto[i] = toupper(buffer_produto[i]);
                strcpy(P.fabricante, buffer_produto);

            mvwprintw(janela_add_produto, getcury(janela_add_produto)+1, LARGURA_MAX_CONSOLE/8, "Unidade: ");
            wrefresh(inserir_dados_produto);
            wrefresh(janela_add_produto);

                wgetnstr(janela_add_produto, buffer_produto, 3);
                if (atoi(buffer_produto) > 0)
                {
                    if (atoi(buffer_produto) == 1)
                        strcpy(P.unidade,"UN");
                    else if (atoi(buffer_produto) == 12)
                        strcpy(P.unidade,"DZ");
                    else
                        strcpy(P.unidade,"PT");
                }
                else
                {
                    for (int i = 0; i < 15; i++)
                        if (islower(buffer_produto[i]))
                            buffer_produto[i] = toupper(buffer_produto[i]);
                    strcpy(P.unidade, buffer_produto);
                }         

            mvwprintw(janela_add_produto, getcury(janela_add_produto)+1, LARGURA_MAX_CONSOLE/8, "Quantidade: ");
            wrefresh(inserir_dados_produto);
            wrefresh(janela_add_produto);

                wgetnstr(janela_add_produto, buffer_produto, 4);
                P.quantidade = atoi(buffer_produto);

            mvwprintw(janela_add_produto, getcury(janela_add_produto)+1, LARGURA_MAX_CONSOLE/8, "Valor Unitário: ");
            wrefresh(inserir_dados_produto);
            wrefresh(janela_add_produto);

                wgetnstr(janela_add_produto, buffer_produto, 7);
                for (int i = 0; i < 8; i++)
                {
                    if (ispunct(buffer_produto[i]))
                    {
                        buffer_produto[i] = '.';
                    }
                }
                P.valor_uni = strtof(buffer_produto, NULL);

                mvwprintw(janela_add_produto, getcury(janela_add_produto)+1, LARGURA_MAX_CONSOLE/8, "Subtotal: ");
                wrefresh(inserir_dados_produto);
                wrefresh(janela_add_produto);

            wgetnstr(janela_add_produto, buffer_produto, 7);
            for (int i = 0; i < 8; i++)
            {
                if (ispunct(buffer_produto[i]))
                {
                    buffer_produto[i] = '.';
                }
            }
            P.subtotal = strtof(buffer_produto, NULL);

                mvwprintw(janela_add_produto, getcury(janela_add_produto)+2, LARGURA_MAX_CONSOLE/8, "%s - %s - %s - %s - %d - %.2f - %.2f",P.id_fabrica,P.nome,P.fabricante,P.unidade,P.quantidade,P.valor_uni,P.subtotal);
            
            wrefresh(janela_add_produto);
            } while (produto_valido);
            getch();
            endwin();

            break;
            return 1;
        case 2:
            break;
            return 1;
        case 3:
            break;
            return 1;
        case 4:
            break;
            return 1;
        case 5:
            break;
            return 1;
        default:
            break;
            return 0;
    }
}

int sql_retorno(void *Inutilizado, int argc, char **argv, char **coluna)
{
    for (int i = 0; i < argc; i++)
    {
        strcat(buffer_sql, argv[i]);
        if (i != argc-1) strcat(buffer_sql, ",");
    }
    return 1;
}

int select_id(sqlite3 *BD, int id, produto *P)
{
    char buffer[50];
    sprintf(buffer, "SELECT * FROM produtos WHERE id = %d", id); 

    sqlite3_exec(BD, buffer, sql_retorno, 0, &erro_sql);

    sscanf(buffer_sql, "%hu,%15[^,],%30[^,],%14[^,],%2[^,],%hu,%f,%f", 
    &P->id, &P->id_fabrica, P->nome, P->fabricante, P->unidade, &P->quantidade, &P->valor_uni, &P->subtotal);

    return 1;
}

int adicionar_produto(sqlite3 *BD, produto P)
{
    char buffer[200];

    sprintf(buffer, "SELECT INTO produtos (id_fabrica,nome,fabricante,unidade,quantidade,valor_uni,subtotal) VALUES (%llu,'%s','%s','%s',%hu,%.2f,%.2f);",
    P.id_fabrica, P.nome, P.fabricante, P.unidade, P.quantidade, P.valor_uni, P.subtotal);

    sqlite3_exec(BD, buffer, sql_retorno, 0, &erro_sql);

    return 1;
}
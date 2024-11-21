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
#define VERSAO_PROGRAMA 0.8

    typedef struct {

        unsigned int id;
        char id_fabrica[16];
        char nome[51];
        char fabricante[15];
        char unidade[3];
        unsigned short int quantidade;
        float valor_uni;
        float subtotal;

    } produto;

    sqlite3 *banco_dados;
    sqlite3_stmt *handler_sql = 0;

    char buffer_sql[120];
    char *erro_sql;

    char nome_empresa[51];
    int total_itens = 0;
    int maior_id = 0;

void colocar_em_maiusculo(char *str);
void virgula_pra_ponto(char *str);
int quantidade_digitos(int num);

int atualizar_total_itens();
int atualizar_maior_id();
int menu(int opc);
int sql_retorno(void *Inutilizado, int argc, char **argv, char **coluna);

int select_id(produto *P, int id);
int adicionar_produto(produto P);
int remover_produto(int id_inicial, int id_final);
int editar_produto(int id_editar, produto P);

int main()
{
//  VARIÁVEIS  

const char nome_pasta[] = "dados_estoque";
const char nome_config[] = "configs";
const char nome_banco[] = "banco_de_dados_estoque";

char caminho_config[50];
sprintf(caminho_config, "%s\\%s.txt", nome_pasta, nome_config);

char caminho_banco[50];
sprintf(caminho_banco, "%s\\%s.db", nome_pasta, nome_banco);

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

    mkdir(nome_pasta);
    FILE *configs = fopen(caminho_config, "r");
    sqlite3_open(caminho_banco, &banco_dados);

    const char criar_tabela[] = "CREATE TABLE IF NOT EXISTS produtos ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                "id_fabrica VARCHAR(15) UNIQUE NOT NULL, "
                                "nome VARCHAR(30), "
                                "fabricante VARCHAR(14), "
                                "unidade CHAR(2), "
                                "quantidade TINYINT, "
                                "valor_uni DECIMAL(5,2)," 
                                "subtotal DECIMAL(5,2) );";

    sqlite3_exec(banco_dados, criar_tabela, sql_retorno, 0, &erro_sql);

    if (configs == NULL)
    {
        fclose(configs);
        configs = fopen(caminho_config, "w+");
        
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

                // sqlite3_exec(banco_dados, "SELECT COUNT(id) FROM produtos;", sql_retorno, 0, &erro_sql);
                // total_itens = atoi(buffer_sql);
                atualizar_total_itens();
                fprintf(configs, buffer_empresa);
                fprintf(configs, ",%d",total_itens);
            
            delwin(inserir_nome_empresa);
            delwin(janela_nome_empresa);

        endwin();
    }
    else fscanf(configs, "%50[^,],%d,%d",nome_empresa,&total_itens,&maior_id);
    
    fclose(configs);

    FILE *produtos = fopen("dados_estoque\\produtos.csv", "r");

    if (produtos != NULL)
    {
        char buffer_csv[255];
        while (fgets(buffer_csv, 254, produtos))
        {
            produto C;
            sscanf(buffer_csv,"%15[^,],%50[^,],%14[^,],%2[^,],%d,%f,%f",
            C.id_fabrica,C.nome,C.fabricante,C.unidade,&C.quantidade,&C.valor_uni,&C.subtotal);

            colocar_em_maiusculo(C.nome);
            colocar_em_maiusculo(C.fabricante);
            colocar_em_maiusculo(C.unidade);

            if (!(C.id_fabrica[0] == '\0' || C.nome[0] == '\0' || C.fabricante[0] == '\0' || C.unidade[0] == '\0' || C.valor_uni <= 0)) 
            {
                adicionar_produto(C);
                atualizar_maior_id();
                remove("dados_estoque\\produtos.csv");
            }
            
        }
    }
    fclose(produtos);

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

        mvwprintw(caixaInfo, ALTURA_MAX_CONSOLE-4, ( InfoX - (quantidade_digitos(total_itens) + 30) )/2, 
        "Quantidade Total de Produtos: %0*d", quantidade_digitos(total_itens), total_itens);

        mvwprintw(tabela_produtos, 2, 0, "├");
        for (int i = 1; i < getmaxx(tabela_produtos)-1; i++)
            mvwprintw(tabela_produtos, 2, i, "─");
        mvwprintw(tabela_produtos, 2, getmaxx(tabela_produtos)-1, "┤");

        if (tabelaX >= 124)
            mvwprintw(tabela_produtos, 1, 1+(tabelaX - 124)/2, " ID │   COD INTERNO   │                        NOME                        │   FABRICANTE   │ UN │ QNTD │ VALOR UN │ SUBTOTAL "); 
        else if (tabelaX < 124)
            mvwprintw(tabela_produtos, 1, 1+(tabelaX - 83)/2, " ID │           NOME              │   FABRICANTE   │ QNTD │ VALOR UN │ SUBTOTAL "); 

        if (total_itens > 0)
        {
            int ids_usados[opcaoMenorY-4], j = 0;
            produto P;

            for (int i = 1; i <= maior_id; i++)
            {
                int retorno_select = select_id(&P, i);
         
                if (retorno_select)
                {
                    ids_usados[j] = P.id;
                    j++;
                }
                if (j == (opcaoMenorY-4)) break;
            }
            if (tabelaX >= 124)
            {         

                for (int i = 0; i < j; i++)
                {         
                    select_id(&P, ids_usados[i]);                  
                    mvwprintw(tabela_produtos, 3+i, 1+(tabelaX - 124)/2," %02d │ %-15.15s │ %-50.50s │ %-14.14s │ %-2.2s │ %-4d │ %-8.2f │ %-8.2f ",
                    P.id,P.id_fabrica,P.nome,P.fabricante,P.unidade,P.quantidade,P.valor_uni,P.subtotal); 
                }
                
                for (int i = getcury(tabela_produtos)+1; i < opcaoMenorY-1; i++)
                    mvwprintw(tabela_produtos, i,1+(tabelaX - 124)/2, "    │                 │                                                    │                │    │      │          │          ",i);
            } 
            else if (tabelaX < 124)
                {
                    for (int i = 0; i < j; i++)
                    {
                        select_id(&P, ids_usados[i]);
                        mvwprintw(tabela_produtos, 3+i, 1+(tabelaX - 83)/2," %02d │ %-27.27s │ %-14s │ %-4hu │ %-8.2f │ %-8.2f ",
                        P.id,P.nome,P.fabricante,P.quantidade,P.valor_uni,P.subtotal);
                    }
                    for (int i = getcury(tabela_produtos)+1; i < opcaoMenorY-1; i++)
                        mvwprintw(tabela_produtos, i, 1+(tabelaX - 83)/2, "    │                             │                │      │          │          "); 
                }
            
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
                    case 27:
                        rodar_programa = 0;
                        break;
                }
            } while (!(tecla_pressionada == 10 || tecla_pressionada == 27));

            delwin(caixa_opcoes_maior);
            delwin(caixa_opcoes_menor);
            delwin(caixaInfo);
            delwin(tabela_produtos);
            endwin();

            if (opcao_selecionada == qntd_opcoes-1 || tecla_pressionada == 27)
                rodar_programa = 0;
            else if (opcao_selecionada < qntd_opcoes)
                menu(opcao_selecionada+1);

//  FIM DO MENU

    } while (rodar_programa);

    configs = fopen(caminho_config,"w");
    fprintf(configs, "%s,%d,%d",nome_empresa,total_itens,maior_id);
    fclose(configs);

    sqlite3_free(handler_sql);
    sqlite3_close(banco_dados);
    
//  RESTO DO MAIN

	return 0;
} 

//  DECLARAÇÃO DAS FUNÇÕES

int menu(int opc)
{
    switch(opc)
    {
        case 1:
        {
            produto P;
            char buffer_produto[51];
            int num_produto = 1, continuar_adicionando = 0, i = 0;

            const int add_produtoX = LARGURA_MAX_CONSOLE/3;
            const int tabelaY = ALTURA_MAX_CONSOLE-16;
            const int InfoX = LARGURA_MAX_CONSOLE-(add_produtoX);
            const int tabelaX = ((17*InfoX)/20);

            initscr();
            WINDOW * janela_tabela = newwin( ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE-add_produtoX, 0, add_produtoX-1);
            WINDOW * janela_add_produto = newwin( ALTURA_MAX_CONSOLE-((ALTURA_MAX_CONSOLE-tabelaY)/2), add_produtoX, ((ALTURA_MAX_CONSOLE-tabelaY)/2), 0 );
            WINDOW * tabela_produtos = newwin( tabelaY, tabelaX, (ALTURA_MAX_CONSOLE-tabelaY)/2, add_produtoX + ((InfoX - tabelaX)/2));
            WINDOW * janela_titulo = newwin( ((ALTURA_MAX_CONSOLE-tabelaY)/2)+1, add_produtoX, 0, 0);
            WINDOW * ler_add_produto = newwin(14, add_produtoX-2, ALTURA_MAX_CONSOLE/4, 1);

            refresh();
            box(tabela_produtos, 0, 0);
            wborder(janela_titulo, 0, 0, 0, 0, 0, 9516, 9500, 9508);
            wborder(janela_tabela, 0, 0, 0, 0, 0, 0, 9524, 0);

            mvwprintw(janela_tabela, (ALTURA_MAX_CONSOLE-tabelaY)/4, (LARGURA_MAX_CONSOLE-add_produtoX-21)/2, "Histórico de Produtos");

            mvwprintw(tabela_produtos, 2, 0, "├");
            for (int i = 1; i < getmaxx(tabela_produtos)-1; i++)
                mvwprintw(tabela_produtos, 2, i, "─");
            mvwprintw(tabela_produtos, 2, getmaxx(tabela_produtos)-1, "┤");

            if (tabelaX >= 124)
                mvwprintw(tabela_produtos, 1, 1+(tabelaX - 124)/2, " ID │   COD INTERNO   │                        NOME                        │   FABRICANTE   │ UN │ QNTD │ VALOR UN │ SUBTOTAL "); 
            else if (tabelaX < 124)
                mvwprintw(tabela_produtos, 1, 1+(tabelaX - 83)/2, " ID │           NOME              │   FABRICANTE   │ QNTD │ VALOR UN │ SUBTOTAL "); 

            box(janela_add_produto, 0, 0);
            
            do {

                wclear(janela_add_produto);
                wclear(ler_add_produto);
                box(janela_add_produto, 0, 0);
                mvwprintw(janela_titulo, (ALTURA_MAX_CONSOLE-tabelaY)/4, (getmaxx(janela_titulo)-24)/2, "ADICIONAR %2d° PRODUTO", num_produto);
                mvwprintw(ler_add_produto, 0, 2, "ID do Fabricante: ");

                wrefresh(janela_add_produto);
                wrefresh(janela_tabela);
                wrefresh(tabela_produtos);
                wrefresh(janela_titulo); 

                    wgetnstr(ler_add_produto, buffer_produto, 15);
                    strcpy(P.id_fabrica, buffer_produto);

                mvwprintw(ler_add_produto, getcury(ler_add_produto)+1, 2, "Nome do Produto: ");
                    wgetnstr(ler_add_produto, buffer_produto, 50);
                    colocar_em_maiusculo(buffer_produto);
                    strcpy(P.nome, buffer_produto);

                mvwprintw(ler_add_produto, getcury(ler_add_produto)+1, 2, "Fabricante: ");
                    wgetnstr(ler_add_produto, buffer_produto, 14);
                    colocar_em_maiusculo(buffer_produto);
                    strcpy(P.fabricante, buffer_produto);

                mvwprintw(ler_add_produto, getcury(ler_add_produto)+1, 2, "Unidade: ");
                    wgetnstr(ler_add_produto, buffer_produto, 2); 
                    colocar_em_maiusculo(buffer_produto);
                    strcpy(P.unidade, buffer_produto);       

                mvwprintw(ler_add_produto, getcury(ler_add_produto)+1, 2, "Quantidade: ");
                    wgetnstr(ler_add_produto, buffer_produto, 4);
                    P.quantidade = atoi(buffer_produto);

                mvwprintw(ler_add_produto, getcury(ler_add_produto)+1, 2, "Valor Unitário: ");
                    wgetnstr(ler_add_produto, buffer_produto, 8);
                    virgula_pra_ponto(buffer_produto);
                    P.valor_uni = strtof(buffer_produto, NULL);

                mvwprintw(ler_add_produto, getcury(ler_add_produto)+1, 2, "Subtotal: ");
                wrefresh(ler_add_produto);
                    wgetnstr(ler_add_produto, buffer_produto, 8);
                    virgula_pra_ponto(buffer_produto);

                if (strtof(buffer_produto, NULL) != P.quantidade*P.valor_uni)
                    P.subtotal = P.quantidade*P.valor_uni;
                else
                    P.subtotal = strtof(buffer_produto, NULL);
                
                if (P.id_fabrica[0] == '\0' || P.nome[0] == '\0' || P.fabricante[0] == '\0' || P.unidade[0] == '\0' || P.valor_uni == 0)
                {
                    mvwprintw(janela_add_produto, getcury(ler_add_produto)+9, 2, "Alguma informação passada do produto está errada");
                    mvwprintw(janela_add_produto, getcury(ler_add_produto)+10, 2, "Por favor tente novamente");
                    wrefresh(janela_add_produto);
                    mvwgetch(janela_add_produto, getcury(janela_add_produto)+1, 2);
                    return 0;
                } 
                
                int deu_certo = adicionar_produto(P);

                if (deu_certo != 1)
                {
                    mvwprintw(janela_add_produto, getcury(janela_add_produto)+3, 2, "Alguma informação passada está errada, Por favor tente novamente");
                    wrefresh(janela_add_produto);
                    mvwgetch(janela_add_produto, getcury(janela_add_produto)+1, 2);
                    return 0;
                }

                if (tabelaX >= 124)
                {
                    mvwprintw(tabela_produtos, 3+i, 1+(tabelaX - 124)/2," %02d │ %-15s │ %-50s │ %-14s │ %-2s │ %-4hu │ %-8.2f │ %-8.2f ",
                    maior_id,P.id_fabrica,P.nome,P.fabricante,P.unidade,P.quantidade,P.valor_uni,P.subtotal);
                    wrefresh(tabela_produtos);
                    i++;
                } 
                else if (tabelaX < 124)
                {
                    mvwprintw(tabela_produtos, 3+i, 1+(tabelaX - 83)/2," %02d │ %-27.27s │ %-14s │ %-4hu │ %-8.2f │ %-8.2f ",
                    maior_id,P.nome,P.fabricante,P.quantidade,P.valor_uni,P.subtotal);
                    wrefresh(tabela_produtos);
                    i++;
                }

                mvwprintw(janela_add_produto, getcury(ler_add_produto)+9, 2, "Deseja adicionar mais produtos?");
                mvwprintw(janela_add_produto, getcury(ler_add_produto)+10, 2, "[S] para Sim  -  [N] para Não");
                char resposta = mvwgetch(janela_add_produto, getcury(janela_add_produto)+1, 16);

                if (tolower(resposta) == 's')
                {
                    continuar_adicionando = 1;
                    num_produto++;
                }
                else continuar_adicionando = 0;

            } while (continuar_adicionando);

            endwin();

            break;
            return 1;
        }
        case 2:
        {
            initscr();
            echo();
            curs_set(1);
            const int janela_idX = (25*LARGURA_MAX_CONSOLE)/100;
            const int janela_idY = (50*ALTURA_MAX_CONSOLE)/100;
            const int metadeX = LARGURA_MAX_CONSOLE/2;
            const int metadeY = ALTURA_MAX_CONSOLE/2;
            const int tituloY = ((ALTURA_MAX_CONSOLE-2)/6)-2;

            WINDOW * janela_externa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
            WINDOW * janela_id = newwin(janela_idY, janela_idX, metadeY-(janela_idY/2), metadeX-(janela_idX/2));
            WINDOW * janela_ler_id = newwin(janela_idY-2, janela_idX-2, metadeY-(janela_idY/2)+1, metadeX-(janela_idX/2)+1);

            refresh();
            box(janela_externa, 0, 0);
            box(janela_id, 0, 0);
            wrefresh(janela_externa);
            wrefresh(janela_id);

            mvwprintw(janela_ler_id, (janela_idY-1)/6, (janela_idX/2)-7, "EDITAR PRODUTO");
            mvwprintw(janela_ler_id, (janela_idY-1)/3, ((janela_idX-1)/2)-20, "Insira o ID do produto que deseja editar");
            wrefresh(janela_ler_id);

            char buffer_id[10];

            mvwgetnstr(janela_ler_id, getcury(janela_ler_id)+2, ((janela_idX-1)/2)-1, buffer_id, 9);

            int id_editar = atoi(buffer_id);
            produto *produto_editar = calloc(1, sizeof(produto));

            if (!id_editar || !(select_id(produto_editar, id_editar)))
            {
                free(produto_editar);
                mvwprintw(janela_ler_id, getcury(janela_ler_id)+2, ((janela_idX-1)/2)-6, "ID Inválido!");
                mvwprintw(janela_ler_id, getcury(janela_ler_id)+1, ((janela_idX-1)/2)-16, "Por favor digite um ID já usado!");
                wrefresh(janela_ler_id);
                wgetch(janela_ler_id);
                return 0;
            }
            free(produto_editar);
            int tamanho_num = strlen(buffer_id);

            delwin(janela_externa);
            delwin(janela_id);
            delwin(janela_ler_id);
            clear();
            refresh();

            WINDOW * janela_esquerda = newwin(ALTURA_MAX_CONSOLE, metadeX, 0, 0);
            WINDOW * janela_direita = newwin(ALTURA_MAX_CONSOLE, metadeX, 0, metadeX-1);
            WINDOW * inserir_novas_infos = newwin(ALTURA_MAX_CONSOLE-2-tituloY, metadeX-2, tituloY, 1);
            WINDOW * titulo = newwin(tituloY, metadeX, 0, 0);
            WINDOW * titulo_id_original = newwin(tituloY, metadeX, 0, metadeX-1);
            refresh();

            box(janela_esquerda, 0, 0);
            wborder(janela_direita, 0, 0, 0, 0, 9516, 0, 9524, 0);
            wborder(titulo, 0, 0, 0, 0, 9516, 0, 9524, 0);
            wborder(titulo_id_original, 0, 0, 0, 0, 9516, 0, 9524, 0);

            produto X; 
            produto Y;
            char buffer_editar[60];
            select_id(&X, id_editar);
            const int id_original_posX = metadeX/5;

            mvwprintw(titulo, (tituloY/2)-1, (metadeX/2)-8, "EDITAR  PRODUTOS");
            mvwprintw(titulo_id_original, (tituloY/2)-1, (metadeX/2)-6, "ID  ORIGINAL");
            mvwprintw(janela_direita, (ALTURA_MAX_CONSOLE-2)/6, (metadeX/2)-4-tamanho_num, "ID: %d", id_editar);
            mvwprintw(janela_direita, getcury(janela_direita)+3, id_original_posX, "ID Fabricante: %s", X.id_fabrica);
            mvwprintw(janela_direita, getcury(janela_direita)+3, id_original_posX, "Nome do Produto: %s", X.nome);
            mvwprintw(janela_direita, getcury(janela_direita)+3, id_original_posX, "Fabricante: %s", X.fabricante);
            mvwprintw(janela_direita, getcury(janela_direita)+3, id_original_posX, "Unidade: %s", X.unidade);
            mvwprintw(janela_direita, getcury(janela_direita)+3, id_original_posX, "Quantidade: %d", X.quantidade);
            mvwprintw(janela_direita, getcury(janela_direita)+3, id_original_posX, "Valor Unitário: %.2f", X.valor_uni);
            mvwprintw(janela_direita, getcury(janela_direita)+3, id_original_posX, "Subtotal: %.2f", X.subtotal);

            wrefresh(janela_direita);
            wrefresh(janela_esquerda);
            wrefresh(titulo);
            wrefresh(titulo_id_original);
            
            mvwprintw(inserir_novas_infos, 5, getbegx(inserir_novas_infos)+2, "ID Fabricante: ");
                wgetnstr(inserir_novas_infos, buffer_editar, 15);
                colocar_em_maiusculo(buffer_editar);
                strcpy(Y.id_fabrica, buffer_editar);
            
            mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, "Nome: ");
                wgetnstr(inserir_novas_infos, buffer_editar, 50);
                colocar_em_maiusculo(buffer_editar);
                strcpy(Y.nome, buffer_editar);

            mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, "Fabricante: ");
                wgetnstr(inserir_novas_infos, buffer_editar, 14);
                colocar_em_maiusculo(buffer_editar);
                strcpy(Y.fabricante, buffer_editar);

            mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, "Unidade: ");
                wgetnstr(inserir_novas_infos, buffer_editar, 2);

                if (atoi(buffer_editar)) strcpy(Y.unidade, "UN");
                else if (atoi(buffer_editar) % 6 == 0) strcpy(Y.unidade, "DZ");
                else if (atoi(buffer_editar) > 10) strcpy(Y.unidade, "PC");
                else 
                {
                    colocar_em_maiusculo(buffer_editar);
                    strcpy(Y.unidade, buffer_editar);
                }

            buffer_editar[0] = '\0';
            mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, "Quantidade: ");
                wgetnstr(inserir_novas_infos, buffer_editar, 3);
                Y.quantidade = atoi(buffer_editar);

            mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, "Valor Unitário: ");
                wgetnstr(inserir_novas_infos, buffer_editar, 5);
                virgula_pra_ponto(buffer_editar);
                Y.valor_uni = atof(buffer_editar);

            mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, "Subtotal: ");
                wgetnstr(inserir_novas_infos, buffer_editar, 5);
                virgula_pra_ponto(buffer_editar);
                Y.subtotal = atof(buffer_editar);

            if (Y.subtotal != Y.quantidade*Y.valor_uni || Y.subtotal == 0)
                Y.subtotal = Y.quantidade*Y.valor_uni;

            mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, "%s - %s - %s - %s - %d - %f - %f", 
            Y.id_fabrica, Y.nome, Y.fabricante, Y.unidade, Y.quantidade, Y.valor_uni, Y.subtotal);

            if (Y.id_fabrica[0] == '\0' || Y.nome[0] == '\0' || Y.fabricante[0] == '\0' || Y.unidade[0] == '\0')
            {
                mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, getbegx(inserir_novas_infos)+2, 
                "Alguma informação passada é inválida, por favor tente novamente");
                wgetch(inserir_novas_infos);
                return 0;
            }
            else
            {
                mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+2, (metadeX/2)-22, "Você tem certeza que quer editar o Produto %d?",id_editar);
                mvwprintw(inserir_novas_infos, getcury(inserir_novas_infos)+1, (metadeX/2)-10, "[S] - Sim   [N] - Não");
                char resposta = mvwgetch(inserir_novas_infos, getcury(inserir_novas_infos)+1, (metadeX/2)-1);

                if (tolower(resposta) == 's') editar_produto(id_editar, Y);
                else return 0;
            }

            wrefresh(inserir_novas_infos);
            wrefresh(titulo);

            getch();

            endwin();

            break;
            return 1;
        }
        case 3:
        {
            initscr();

            const int quadrado_removerY = (3*ALTURA_MAX_CONSOLE)/5;
            const int quadrado_removerX = 50;
            const int quadrado_pY = ALTURA_MAX_CONSOLE/4;
            const int tituloY = (7*ALTURA_MAX_CONSOLE)/63;
            const int tituloX = (40*LARGURA_MAX_CONSOLE)/273;
            const int distX = LARGURA_MAX_CONSOLE/10;

            WINDOW * janela_externa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);

            WINDOW * janela_remover_produtos = newwin(quadrado_removerY, quadrado_removerX, quadrado_pY, distX);
            WINDOW * janela_info = newwin(quadrado_removerY, quadrado_removerX, quadrado_pY, LARGURA_MAX_CONSOLE-(quadrado_removerX+distX));

            WINDOW * janela_titulo = newwin(tituloY, tituloX, 2, (LARGURA_MAX_CONSOLE-tituloX)/2);
            WINDOW * ler_remover = newwin(quadrado_removerY-2, quadrado_removerX-2, getbegy(janela_remover_produtos)+1, getbegx(janela_remover_produtos)+1);

            refresh();

            box(janela_externa, 0, 0);
            box(janela_remover_produtos, 0, 0);
            box(janela_info, 0, 0);
            box(janela_titulo, 0, 0);

            mvwprintw(janela_titulo, tituloY/2, (tituloX-17)/2, "REMOVER  PRODUTOS");
            mvwprintw(ler_remover, quadrado_removerY/10, (quadrado_removerX-18)/2, "Tipos de Remoção");

            if (total_itens == 0 || maior_id == 0)
                mvwprintw(janela_info, quadrado_removerY/10, 5, "Não há nenhum Produto registrado no estoque");
            else
            {
                mvwprintw(janela_info, quadrado_removerY/10, 5, "Há %d Produtos registrados em estoque", total_itens);
                mvwprintw(janela_info, getcury(janela_info)+3, 5, "Maior ID de produto registrado: %d", maior_id);
            }
            
            wrefresh(janela_externa);
            wrefresh(janela_remover_produtos);
            wrefresh(janela_info);
            wrefresh(janela_titulo);

            const int tipos_remover = 2;
            char lista_remover[2][30] = {{"Remover 1 produto"},{"Remover vários produtos"}};
            int tecla_pressionada = 0, opc_selecionada = 0, y_atual = getcury(ler_remover);

            keypad(ler_remover, true);
            curs_set(0);
            noecho();

            while (!(tecla_pressionada == 10 || tecla_pressionada == 27))
            {   
                for (int i = 0; i < tipos_remover; i++)
                {
                    if (i == opc_selecionada) wattron(ler_remover, A_STANDOUT);
                    mvwprintw(ler_remover, y_atual+4+i*3, 4, lista_remover[i]);
                    wattroff(ler_remover, A_STANDOUT);
                }
                wrefresh(ler_remover);

                tecla_pressionada = wgetch(ler_remover);

                switch(tecla_pressionada)
                {
                    case KEY_UP:
                        opc_selecionada--;
                        if (opc_selecionada < 0) opc_selecionada = 1;
                        break;
                    case KEY_DOWN:
                        opc_selecionada++;
                        if (opc_selecionada > tipos_remover-1) opc_selecionada = 0;
                        break;
                }

            }

            echo();

            char buffer_rem[21];

            if (tecla_pressionada != 27)
            {
                if (opc_selecionada == 0)
                {

                    curs_set(1);
                    wclear(ler_remover);
                    mvwprintw(ler_remover, quadrado_removerY/10, (quadrado_removerX-20)/2, "Remover um Produto");
                    wrefresh(ler_remover);

                    mvwprintw(ler_remover, getcury(ler_remover)+4, 2, "Digite o ID do produto: ");
                    wgetnstr(ler_remover, buffer_rem, 21);
                    wrefresh(ler_remover);
                    
                    int id_rem = atoi(buffer_rem);
                    
                    if (id_rem <= 0)
                    {
                        mvwprintw(ler_remover, getcury(ler_remover)+2, 2, "ID inválido, por favor insira outro!");
                        wrefresh(ler_remover);
                        wgetch(ler_remover);
                    }
                    else
                    {
                        mvwprintw(ler_remover, getcury(ler_remover)+2, 2, "Você tem certeza que quer");
                        mvwprintw(ler_remover, getcury(ler_remover)+1, 2, "apagar o ID %d? [S] / [N]", id_rem);
                        char resposta = mvwgetch(ler_remover, getcury(ler_remover)+2, 4);
                        wrefresh(ler_remover);

                        if (tolower(resposta) == 's' && total_itens > 0)
                        {
                            int retorno = remover_produto(id_rem, id_rem);
                            if (id_rem == maior_id) atualizar_maior_id();
                            total_itens--;
                        }
                            
                    }

                }
                else if (opc_selecionada == 1)
                {
                    curs_set(1);
                    wclear(ler_remover);
                    mvwprintw(ler_remover, y_atual, (quadrado_removerX-30)/2, "Remover um grupo de Produtos");
                    wrefresh(ler_remover);

                    mvwprintw(ler_remover, getcury(ler_remover)+4, 2, "Digite o ID inicial: ");
                    wgetnstr(ler_remover, buffer_rem, 21);
                    wrefresh(ler_remover);
                    
                    int id_inicio_rem = atoi(buffer_rem);

                    if (id_inicio_rem <= 0)
                    {
                        mvwprintw(ler_remover, getcury(ler_remover)+2, 2, "ID inválido, por favor insira outro!");
                        wrefresh(ler_remover);
                        wgetch(ler_remover);
                    }
                    else
                    {
                        buffer_rem[0] = '\0';
                        mvwprintw(ler_remover, getcury(ler_remover)+2, 2, "Digite o ID final: ");
                        wgetnstr(ler_remover, buffer_rem, 21);
                        wrefresh(ler_remover);

                        int id_final_rem = atoi(buffer_rem);

                        if (id_final_rem <= 0 || (id_final_rem < id_inicio_rem))
                        {
                            mvwprintw(ler_remover, getcury(ler_remover)+2, 2, "ID inválido, por favor insira outro!");
                            wrefresh(ler_remover);
                            wgetch(ler_remover);
                        }
                        else
                        {
                            mvwprintw(ler_remover, getcury(ler_remover)+2, 2, "Você tem certeza que quer");
                            mvwprintw(ler_remover, getcury(ler_remover)+1, 2, "apagar todos os IDs de %d até %d? [S] / [N]", id_inicio_rem, id_final_rem);
                            wrefresh(ler_remover);
                            char resposta = mvwgetch(ler_remover, getcury(ler_remover)+2, 2);

                            if (tolower(resposta) == 's')
                            {                            
                                if (total_itens > 0)
                                {
                                    remover_produto(id_inicio_rem, id_final_rem);
                                        
                                    atualizar_total_itens();
                                    atualizar_maior_id();
                                }
                            }
                        }

                        
                            
                }

                }

            }

            endwin();
            break;
            return 1;
        }
        case 4:
        {
            initscr();
            noecho();
            cbreak();
            curs_set(0);

            const int tabelaX = 145;
            const int tabelaY = 4*ALTURA_MAX_CONSOLE/5;
            const int tabela_posX = (LARGURA_MAX_CONSOLE-tabelaX)/2;
            const int tabela_posY = (ALTURA_MAX_CONSOLE-tabelaY)/2;
            
            const int produtos_por_pagina = tabelaY-4;
            int total_paginas = total_itens/produtos_por_pagina;
            int pagina_atual = 0;

            if (total_itens % produtos_por_pagina != 0) total_paginas++;

            int tecla_pressionada;
            const int posX_seta_direita = tabelaX+((41*LARGURA_MAX_CONSOLE)/237);
            const int posX_seta_esquerda = tabela_posX+1;
            const int posY_barra = tabelaY+tabela_posY;
            const int digitos_paginas = quantidade_digitos(total_paginas);

            do {

            WINDOW * janela_externa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
            WINDOW * tabela_produtos = newwin(tabelaY, tabelaX, tabela_posY, tabela_posX);
            keypad(janela_externa, TRUE);
            refresh();

            box(janela_externa, 0, 0);
            wborder(tabela_produtos, 9553, 9553, 9552, 9552, 9556, 9559, 9562, 9565);

            mvwprintw(janela_externa, tabela_posY-3, (LARGURA_MAX_CONSOLE-18)/2, "TABELA DE PRODUTOS");
            mvwprintw(tabela_produtos, 2, 0, "╠");
            for (int i = 1; i < tabelaX-1; i++)
                mvwprintw(tabela_produtos, 2, i, "═");
            mvwprintw(tabela_produtos, 2, tabelaX-1, "╣");     
            mvwprintw(tabela_produtos, 1, 1, 
            " ID ┃ CÓDIGO  FABRICA ┃                        NOME                        ┃   FABRICANTE   ┃ UNIDADE ┃ QUANTIDADE ┃ VALOR UNITÁRIO ┃ SUBTOTAL ");
            
            start_color();

            if (total_itens > 0)
            {
                int ids_usados[tabelaY-4], j = 0;
                produto P;

                for (int i = 1+(pagina_atual*(tabelaY-4)); i <= maior_id; i++)
                {
                    int retorno_select = select_id(&P, i);
            
                    if (retorno_select)
                    {
                        ids_usados[j] = P.id;
                        j++;
                    }
                    if (j == tabelaY-4) break;
                }

                for (int i = 0; i < j; i++)
                {         
                    select_id(&P, ids_usados[i]);                  
                    mvwprintw(tabela_produtos, 3+i, 1," %2d ┃ %15s ┃ %50s ┃ %14s ┃ %7s ┃ %10d ┃ %14.2f ┃ %5.2f ", 
                    P.id, P.id_fabrica, P.nome, P.fabricante, P.unidade, P.quantidade, P.valor_uni, P.subtotal); 
                }
                    
                for (int i = getcury(tabela_produtos)+1; i < tabelaY-1; i++)
                    mvwprintw(tabela_produtos, i, 1, "    ┃                 ┃                                                    ┃                ┃         ┃            ┃                ┃          ");
                
            }

            mvwprintw(janela_externa, posY_barra, posX_seta_esquerda, " <- ");
            mvwprintw(janela_externa, posY_barra, posX_seta_direita, " -> ");
            mvwprintw(janela_externa, posY_barra, (LARGURA_MAX_CONSOLE-(20+(2*digitos_paginas)))/2, "┃ PÁGINA %0*d   /   %0*d ┃",
            digitos_paginas, pagina_atual+1, digitos_paginas, total_paginas);
            mvwchgat(janela_externa, posY_barra, tabela_posX, tabelaX , A_REVERSE, 0, NULL);

            mvwprintw(janela_externa, getcury(janela_externa)+2, (LARGURA_MAX_CONSOLE-26)/2, "[ESC] - Voltar para o Menu");
            wrefresh(janela_externa);
            wrefresh(tabela_produtos);

            tecla_pressionada = wgetch(janela_externa);
            
            switch(tecla_pressionada)
            {
                case KEY_RIGHT:
                    pagina_atual++;
                    if (pagina_atual > total_paginas-1) pagina_atual = 0;
                    break;
                case KEY_LEFT:
                    pagina_atual--;
                    if (pagina_atual < 0) pagina_atual = total_paginas-1;
                    break;
            }

            } while (tecla_pressionada != 27);

            endwin();
            break;
            return 1;
        }
        case 5:
        {   
            initscr();
            noecho();
            cbreak();
            curs_set(0);

            const int selecionarY = ALTURA_MAX_CONSOLE/2;
            const int selecionarX = LARGURA_MAX_CONSOLE/4;
            const int selecionar_pY = (ALTURA_MAX_CONSOLE-selecionarY)/2;
            const int selecionar_pX = (LARGURA_MAX_CONSOLE-selecionarX)/2;

            WINDOW * janela_externa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
            WINDOW * janela_selecionar_filtro = newwin(selecionarY, selecionarX, selecionar_pY, selecionar_pX);
            keypad(janela_selecionar_filtro, TRUE);
            refresh();

            char filtros[][30] = {
                {"Quantidade Zero"},
                {"Quantidade Menor que"},
                {"Quantidade Maior que"},
                {"Valor Menor que"},
                {"Valor Maior que"}
            };

            int qntd_filtros = sizeof(filtros)/sizeof(filtros[0]), tecla_pressionada , opc_selecionada = 0; 

            box(janela_externa, 0, 0);
            box(janela_selecionar_filtro, 0, 0);

            mvwaddstr(janela_selecionar_filtro, selecionarY/8, (selecionarX-16)/2, "LISTA DE FILTROS");
            mvwaddstr(janela_selecionar_filtro, selecionarY-3, (selecionarX-26)/2, "[ESC] - Voltar para o Menu");

            wrefresh(janela_externa);
            wrefresh(janela_selecionar_filtro);

            do {

                for (int i = 0; i < qntd_filtros; i++)
                {
                    if (i == opc_selecionada) wattron(janela_selecionar_filtro, A_STANDOUT);
                    mvwprintw(janela_selecionar_filtro, (selecionarY/5)+(i*2), 3, "%s [X]", filtros[i]);
                    wattroff(janela_selecionar_filtro, A_STANDOUT);
                }

                tecla_pressionada = wgetch(janela_selecionar_filtro);

                switch(tecla_pressionada)
                {
                    case KEY_UP:
                        opc_selecionada--;
                        if (opc_selecionada < 0) opc_selecionada = qntd_filtros-1;
                        break;
                    case KEY_DOWN:
                        opc_selecionada++;
                        if (opc_selecionada > qntd_filtros-1) opc_selecionada = 0;
                        break;
                }

            } while (!(tecla_pressionada == 10 || tecla_pressionada == 27));

            wclear(janela_selecionar_filtro);
            box(janela_selecionar_filtro, 0, 0);

            WINDOW * ler_condicao = newwin(selecionarY-2, selecionarX-2, selecionar_pY+1, selecionar_pX+1);
            char buffer_condicao[20];
            float numero_condicao;

                if (tecla_pressionada == 27)
                {
                    delwin(janela_selecionar_filtro);
                    delwin(janela_externa);
                    endwin();
                    return 1;
                }

                if (opc_selecionada > 0)
                {
                    echo();
                    curs_set(1);
                    
                    mvwprintw(ler_condicao, (selecionarY/8)-1, ((selecionarX-16)/2), "CRIAR CONDIÇÃO");
                    mvwprintw(ler_condicao, getcury(ler_condicao)+2, 3, "%s: ",filtros[opc_selecionada]);

                    wgetnstr(ler_condicao, buffer_condicao, 20);

                    if (isdigit(buffer_condicao[0])) numero_condicao = atof(buffer_condicao);
                    else
                    {
                    mvwprintw(ler_condicao, getcury(ler_condicao)+2, ((selecionarX-44)/2), "Condição inválida! Por favor tente novamente");
                    wgetch(ler_condicao);
                    }
                    wrefresh(ler_condicao);

                    wclear(ler_condicao);
                    wclear(janela_selecionar_filtro);
                    delwin(ler_condicao);
                    delwin(janela_selecionar_filtro);
                } 
                else if (opc_selecionada == 0) numero_condicao = 0;

            int total_itens_filtro;

                if (opc_selecionada < 3)
                {
                    int retorno_filtro;

                    switch(opc_selecionada)
                    {
                        case 0:
                            retorno_filtro = sqlite3_prepare_v2(banco_dados, "SELECT COUNT(id) FROM produtos WHERE quantidade = ?", -1, &handler_sql, 0 );
                            break;
                        case 1:
                            retorno_filtro = sqlite3_prepare_v2(banco_dados, "SELECT COUNT(id) FROM produtos WHERE quantidade < ?", -1, &handler_sql, 0 );
                            break;
                        case 2:
                            retorno_filtro = sqlite3_prepare_v2(banco_dados, "SELECT COUNT(id) FROM produtos WHERE quantidade > ?", -1, &handler_sql, 0 );
                            break;
                    }

                    retorno_filtro = sqlite3_bind_int( handler_sql, 1, (int)numero_condicao);
                    retorno_filtro = sqlite3_step( handler_sql );

                    if (retorno_filtro == SQLITE_ROW) total_itens_filtro = atoi(sqlite3_column_text(handler_sql, 0 ));
                } 
                else
                {
                    int retorno_filtro;

                    switch(opc_selecionada)
                    {
                        case 3:
                            retorno_filtro = sqlite3_prepare_v2(banco_dados, "SELECT COUNT(id) FROM produtos WHERE valor_uni < ?", -1, &handler_sql, 0 );
                            break;
                        case 4:
                            retorno_filtro = sqlite3_prepare_v2(banco_dados, "SELECT COUNT(id) FROM produtos WHERE valor_uni > ?", -1, &handler_sql, 0 );
                            break;
                    }

                    retorno_filtro = sqlite3_bind_double( handler_sql, 1, numero_condicao);
                    retorno_filtro = sqlite3_step( handler_sql );

                    if (retorno_filtro == SQLITE_ROW) total_itens_filtro = atoi(sqlite3_column_text(handler_sql, 0 ));
                }
   
            const int tabelaX = 145;
            const int tabelaY = 4*ALTURA_MAX_CONSOLE/5;
            const int tabela_posX = (LARGURA_MAX_CONSOLE-tabelaX)/2;
            const int tabela_posY = (ALTURA_MAX_CONSOLE-tabelaY)/2;
            const int posX_seta_direita = tabelaX+((41*LARGURA_MAX_CONSOLE)/237);
            const int posX_seta_esquerda = tabela_posX+1;
            const int posY_barra = tabelaY+tabela_posY;
            const int produtos_por_pagina = tabelaY-4;

            int total_paginas = total_itens_filtro/produtos_por_pagina;

            if (total_itens_filtro % produtos_por_pagina != 0) total_paginas++;

            const int digitos_paginas = quantidade_digitos(total_paginas)+1;

            int pagina_atual = 0;

            do {
            
            WINDOW * tabela_produtos = newwin(tabelaY, tabelaX, tabela_posY, tabela_posX);
            keypad(janela_externa, TRUE);
            noecho();
            curs_set(0);

            refresh();
            wborder(tabela_produtos, 9553, 9553, 9552, 9552, 9556, 9559, 9562, 9565);

                switch(opc_selecionada)
                {
                    int tamanho_titulo;
                    
                    case 0:
                        mvwprintw(janela_externa, tabela_posY-3, (LARGURA_MAX_CONSOLE-30)/2, "PRODUTOS COM QUANTIDADE ZERADA");
                        break;
                    case 1:
                        tamanho_titulo = 34+quantidade_digitos((int)numero_condicao);

                        mvwprintw(janela_externa, tabela_posY-3, (LARGURA_MAX_CONSOLE-tamanho_titulo)/2, "PRODUTOS COM QUANTIDADE MENOR QUE %d", (int)numero_condicao);
                        break;
                    case 2:
                        tamanho_titulo = 34+quantidade_digitos((int)numero_condicao);

                        mvwprintw(janela_externa, tabela_posY-3, (LARGURA_MAX_CONSOLE-tamanho_titulo)/2, "PRODUTOS COM QUANTIDADE MAIOR QUE %d", (int)numero_condicao);
                        break;
                    case 3:
                        tamanho_titulo = 29+quantidade_digitos((int)numero_condicao);

                        mvwprintw(janela_externa, tabela_posY-3, (LARGURA_MAX_CONSOLE-tamanho_titulo)/2, "PRODUTOS COM VALOR MENOR QUE %.2f", numero_condicao);
                        break;
                    case 4:
                        tamanho_titulo = 29+quantidade_digitos((int)numero_condicao);

                        mvwprintw(janela_externa, tabela_posY-3, (LARGURA_MAX_CONSOLE-tamanho_titulo)/2, "PRODUTOS COM VALOR MAIOR QUE %.2f", numero_condicao);
                        break;
                }
                
            mvwprintw(tabela_produtos, 2, 0, "╠");
            for (int i = 1; i < tabelaX-1; i++)
                mvwprintw(tabela_produtos, 2, i, "═");
            mvwprintw(tabela_produtos, 2, tabelaX-1, "╣");     
            mvwprintw(tabela_produtos, 1, 1, 
            " ID ┃ CÓDIGO  FABRICA ┃                        NOME                        ┃   FABRICANTE   ┃ UNIDADE ┃ QUANTIDADE ┃ VALOR UNITÁRIO ┃ SUBTOTAL ");
            
                if (total_itens_filtro > 0)
                {
                    int ids_usados[tabelaY-4], j = 0;
                    produto P;

                    switch(opc_selecionada)
                    {
                        case 0:
                            for (int i = 1+(pagina_atual*(tabelaY-4)); i <= maior_id; i++)
                            {
                                int retorno_select_filtro = select_id(&P, i);
                        
                                if (retorno_select_filtro && P.quantidade == 0)
                                {
                                    ids_usados[j] = P.id;
                                    j++;
                                }
                                if (j == tabelaY-4) break;
                            }
                            break;
                        case 1:
                            for (int i = 1+(pagina_atual*(tabelaY-4)); i <= maior_id; i++)
                            {
                                int retorno_select_filtro = select_id(&P, i);
                        
                                if (retorno_select_filtro && P.quantidade < (int)numero_condicao)
                                {
                                    ids_usados[j] = P.id;
                                    j++;
                                }
                                if (j == tabelaY-4) break;
                            }
                            break;
                        case 2:
                            for (int i = 1+(pagina_atual*(tabelaY-4)); i <= maior_id; i++)
                            {
                                int retorno_select_filtro = select_id(&P, i);
                        
                                if (retorno_select_filtro && P.quantidade > (int)numero_condicao)
                                {
                                    ids_usados[j] = P.id;
                                    j++;
                                }
                                if (j == tabelaY-4) break;
                            }
                            break;
                        case 3:
                            for (int i = 1+(pagina_atual*(tabelaY-4)); i <= maior_id; i++)
                            {
                                int retorno_select_filtro = select_id(&P, i);
                        
                                if (retorno_select_filtro && P.valor_uni < numero_condicao)
                                {
                                    ids_usados[j] = P.id;
                                    j++;
                                }
                                if (j == tabelaY-4) break;
                            }
                            break;
                        case 4:
                            for (int i = 1+(pagina_atual*(tabelaY-4)); i <= maior_id; i++)
                            {
                                int retorno_select_filtro = select_id(&P, i);
                        
                                if (retorno_select_filtro && P.valor_uni > numero_condicao)
                                {
                                    ids_usados[j] = P.id;
                                    j++;
                                }
                                if (j == tabelaY-4) break;
                            }
                            break;
                    }

                    for (int i = 0; i < j; i++)
                    {         
                        select_id(&P, ids_usados[i]);                  
                        mvwprintw(tabela_produtos, 3+i, 1," %2d ┃ %15s ┃ %50s ┃ %14s ┃ %7s ┃ %10d ┃ %14.2f ┃ %5.2f ", 
                        P.id, P.id_fabrica, P.nome, P.fabricante, P.unidade, P.quantidade, P.valor_uni, P.subtotal); 
                        // mvwprintw(tabela_produtos, 3+i, 1, "%d", ids_usados[i]);
                    }
                        
                    for (int i = getcury(tabela_produtos)+1; i < tabelaY-1; i++)
                        mvwprintw(tabela_produtos, i, 1, "    ┃                 ┃                                                    ┃                ┃         ┃            ┃                ┃          ");
                    
                }

            mvwprintw(janela_externa, posY_barra, posX_seta_esquerda, " <- ");
            mvwprintw(janela_externa, posY_barra, posX_seta_direita, " -> ");
            mvwprintw(janela_externa, posY_barra, (LARGURA_MAX_CONSOLE-(20+(2*digitos_paginas)))/2, "┃ PÁGINA %0*d   /   %0*d ┃",
            digitos_paginas, pagina_atual+1, digitos_paginas, total_paginas);
            mvwchgat(janela_externa, posY_barra, tabela_posX, tabelaX , A_REVERSE, 0, NULL);

            mvwprintw(janela_externa, getcury(janela_externa)+2, (LARGURA_MAX_CONSOLE-26)/2, "[ESC] - Voltar para o Menu");

            // mvwprintw(janela_externa, 3, 3, "num condicao: %f", numero_condicao);
            // mvwprintw(janela_externa, 4, 3, "total itens: %d", total_itens_filtro);
            // mvwprintw(janela_externa, 5, 3, "digitos: %d", quantidade_digitos((int)numero_condicao));

            wrefresh(janela_externa);
            wrefresh(tabela_produtos);

            tecla_pressionada = wgetch(janela_externa);

            switch(tecla_pressionada)
            {
                case KEY_RIGHT:
                    pagina_atual++;
                    if (pagina_atual > total_paginas-1) pagina_atual = 0;
                    break;
                case KEY_LEFT:
                    pagina_atual--;
                    if (pagina_atual < 0) pagina_atual = total_paginas-1;
                    break;
            }

            } while (tecla_pressionada != 27);
            
            endwin();

            break;
            return 1;
        
        }
    }
}

int sql_retorno(void *Inutilizado, int argc, char **argv, char **coluna)
{
    for (int i = 0; i < argc; i++)
    {
        strcat(buffer_sql, argv[i]);
        if (i != argc-1) strcat(buffer_sql, ",");
    }
    return 0;
}

int quantidade_digitos(int num)
{

    char buffer_digitos[21];

    sprintf(buffer_digitos, "%d",num);
    int qntd_digitos = strlen(buffer_digitos);

    return qntd_digitos; 
}

void colocar_em_maiusculo(char *str)
{
    int tamanho_str = strlen(str);

    for (int i = 0; i < tamanho_str; i++)
        if (islower(str[i])) str[i] = toupper(str[i]);
}

void virgula_pra_ponto(char *str)
{
    int tamanho_str = strlen(str);

    for (int i = 0; i < tamanho_str; i++)
        if (str[i] == ',') str[i] = '.';
}

int atualizar_total_itens()
{
    char *comando_count = "SELECT COUNT(id) FROM produtos;";

    int retorno_count = sqlite3_prepare_v2(banco_dados, comando_count, -1, &handler_sql, 0);

    if (retorno_count != SQLITE_OK)
        return 0;

    retorno_count = sqlite3_step(handler_sql);

    if (retorno_count == SQLITE_ROW)
    {
        total_itens = atoi(sqlite3_column_text(handler_sql, 0 ));
        return 1;
    }
    else 
    {
        return 0;
    }
    
}

int atualizar_maior_id()
{
    char *comando_max = "SELECT MAX(id) FROM produtos;";

    int retorno_max = sqlite3_prepare_v2(banco_dados, comando_max, -1, &handler_sql, 0);

    if (retorno_max != SQLITE_OK)
        return 0;

    retorno_max = sqlite3_step(handler_sql);

    if (retorno_max == SQLITE_ROW)
    {
        maior_id = atoi(sqlite3_column_text(handler_sql, 0 ));
        return 1;
    }
    else 
    {
        return 0;
    }
}    

int select_id(produto *P, int id)
{
    char *comando_select = "SELECT * FROM produtos WHERE ID=?";

    int retorno_select = sqlite3_prepare_v2(banco_dados, comando_select, -1, &handler_sql, 0 );

    if ( retorno_select != SQLITE_OK )
        return 1;

    retorno_select = sqlite3_bind_int( handler_sql, 1, id );

    if ( retorno_select != SQLITE_OK )
        return 1;

    retorno_select = sqlite3_step( handler_sql );

        if ( retorno_select == SQLITE_ROW)
        {
            P->id = atoi(sqlite3_column_text(handler_sql, 0 ));
            strcpy(P->id_fabrica, sqlite3_column_text(handler_sql, 1 ));
            strcpy(P->nome, sqlite3_column_text(handler_sql, 2 ));
            strcpy(P->fabricante, sqlite3_column_text(handler_sql, 3 ));
            strcpy(P->unidade, sqlite3_column_text(handler_sql, 4 ));
            P->quantidade = atoi(sqlite3_column_text(handler_sql, 5 ));
            P->valor_uni = atof((sqlite3_column_text(handler_sql, 6 )));
            P->subtotal = atof((sqlite3_column_text(handler_sql, 7 )));
            return 1;
        } else return 0;    
    
}

int adicionar_produto(produto P)
{
    char *comando_insert = "INSERT INTO produtos (id_fabrica,nome,fabricante,unidade,quantidade,valor_uni,subtotal)"
                           "VALUES (@id_fabrica,@nome,@fabricante,@unidade,@quantidade,@valor_uni,@subtotal);";
    
    int retorno_insert = sqlite3_prepare_v2(banco_dados, comando_insert, -1, &handler_sql, 0);

    if (retorno_insert != SQLITE_OK)
        return 0;
    
    const int pos_id_fabrica = sqlite3_bind_parameter_index( handler_sql, "@id_fabrica");
    const int pos_nome = sqlite3_bind_parameter_index( handler_sql, "@nome");
    const int pos_fabricante = sqlite3_bind_parameter_index( handler_sql, "@fabricante");
    const int pos_unidade = sqlite3_bind_parameter_index( handler_sql, "@unidade");
    const int pos_quantidade = sqlite3_bind_parameter_index( handler_sql, "@quantidade");
    const int pos_valor_uni = sqlite3_bind_parameter_index( handler_sql, "@valor_uni");
    const int pos_subtotal = sqlite3_bind_parameter_index( handler_sql, "@subtotal");

    retorno_insert = sqlite3_bind_text(handler_sql, pos_id_fabrica, P.id_fabrica, -1, NULL);
        if (retorno_insert != SQLITE_OK)
            return 0;

    retorno_insert = sqlite3_bind_text(handler_sql, pos_nome, P.nome, -1, NULL);
        if (retorno_insert != SQLITE_OK)
            return 0;

    retorno_insert = sqlite3_bind_text(handler_sql, pos_fabricante, P.fabricante, -1, NULL);
        if (retorno_insert != SQLITE_OK)
            return 0;

    retorno_insert = sqlite3_bind_text(handler_sql, pos_unidade, P.unidade, -1, NULL);
        if (retorno_insert != SQLITE_OK)
            return 0;
        
    retorno_insert = sqlite3_bind_int(handler_sql, pos_quantidade, P.quantidade);
        if (retorno_insert != SQLITE_OK)
            return 0;
    
    retorno_insert = sqlite3_bind_double(handler_sql, pos_valor_uni, P.valor_uni);
        if (retorno_insert != SQLITE_OK)
            return 0;

    retorno_insert = sqlite3_bind_double(handler_sql, pos_subtotal, P.subtotal);
        if (retorno_insert != SQLITE_OK)
            return 0;

    retorno_insert = sqlite3_step(handler_sql);

    if (retorno_insert != SQLITE_DONE)
        return 0;
    else
    {
        total_itens+=1;
        maior_id+=1;
        return 1;
    } 
    
}

int remover_produto(int id_inicial, int id_final)
{
    char *comando_delete = "DELETE FROM produtos WHERE id >= @id_inicial AND id <= @id_final;";

    int retorno_delete = sqlite3_prepare_v2( banco_dados, comando_delete, -1, &handler_sql, 0 );

    if (retorno_delete != SQLITE_OK)
        return 0;
    
    const int pos_id_inicio = sqlite3_bind_parameter_index( handler_sql, "@id_inicial");
    const int pos_id_fim = sqlite3_bind_parameter_index( handler_sql, "@id_final");

    retorno_delete = sqlite3_bind_int( handler_sql, pos_id_inicio, id_inicial );

    if (retorno_delete != SQLITE_OK)
        return 0;

    retorno_delete = sqlite3_bind_int( handler_sql, pos_id_fim, id_final );

    if (retorno_delete != SQLITE_OK)
        return 0;    

    retorno_delete = sqlite3_step( handler_sql );

    if (retorno_delete == SQLITE_DONE)
        return 1;
    else 
        return 0;

    
}

int editar_produto(int id_editar, produto P)
{
    char *comando_update = "UPDATE produtos SET "
                           "id_fabrica = @id_fabrica,"
                           "nome = @nome,"
                           "fabricante = @fabricante,"
                           "unidade = @unidade,"
                           "quantidade = @quantidade,"
                           "valor_uni = @valor_uni,"
                           "subtotal = @subtotal "
                           "WHERE id = @id_editar;";

    int retorno_update = sqlite3_prepare_v2( banco_dados, comando_update, -1, &handler_sql, 0 );

    if (retorno_update != SQLITE_OK)
        return 0;
    
    const int pos_id_fabricante = sqlite3_bind_parameter_index( handler_sql, "@id_fabrica");
    const int pos_nome = sqlite3_bind_parameter_index( handler_sql, "@nome");
    const int pos_fabricante = sqlite3_bind_parameter_index( handler_sql, "@fabricante");
    const int pos_unidade = sqlite3_bind_parameter_index( handler_sql, "@unidade");
    const int pos_quantidade = sqlite3_bind_parameter_index( handler_sql, "@quantidade");
    const int pos_valor_uni = sqlite3_bind_parameter_index( handler_sql, "@valor_uni");
    const int pos_subtotal = sqlite3_bind_parameter_index( handler_sql, "@subtotal");
    const int pos_id_editar = sqlite3_bind_parameter_index( handler_sql, "@id_editar");

    retorno_update = sqlite3_bind_text(handler_sql, pos_id_fabricante, P.id_fabrica, -1, NULL);
        if (retorno_update != SQLITE_OK)
            return -1;

    retorno_update = sqlite3_bind_text(handler_sql, pos_nome, P.nome, -1, NULL);
        if (retorno_update != SQLITE_OK)
            return -2;

    retorno_update = sqlite3_bind_text(handler_sql, pos_fabricante, P.fabricante, -1, NULL);
        if (retorno_update != SQLITE_OK)
            return -3;
    
    retorno_update = sqlite3_bind_text(handler_sql, pos_unidade, P.unidade, -1, NULL);
        if (retorno_update != SQLITE_OK)
            return -4;
    
    retorno_update = sqlite3_bind_int( handler_sql, pos_quantidade, P.quantidade );
        if (retorno_update != SQLITE_OK)
            return -5;
    
    retorno_update = sqlite3_bind_double( handler_sql, pos_valor_uni, P.valor_uni );
        if (retorno_update != SQLITE_OK)
            return -6;

    retorno_update = sqlite3_bind_double( handler_sql, pos_subtotal, P.subtotal );
        if (retorno_update != SQLITE_OK)
            return -7;

    retorno_update = sqlite3_bind_int( handler_sql, pos_id_editar, id_editar );
        if (retorno_update != SQLITE_OK)
            return -8;

    retorno_update = sqlite3_step( handler_sql );

    if (retorno_update == SQLITE_DONE)
        return 1;
    else 
        return -9;
}
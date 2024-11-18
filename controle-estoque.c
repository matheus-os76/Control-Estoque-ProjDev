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
#define VERSAO_PROGRAMA 0.2

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

                sqlite3_exec(banco_dados, "SELECT COUNT(id) FROM produtos;", sql_retorno, 0, &erro_sql);
                total_itens = atoi(buffer_sql);
                fprintf(configs, buffer_empresa);
                fprintf(configs, ",%d",total_itens);
            
            delwin(inserir_nome_empresa);
            delwin(janela_nome_empresa);

        endwin();
    }
    else fscanf(configs, "%50[^,],%d,%d",nome_empresa,&total_itens,&maior_id);
    
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

        if (tabelaX >= 124)
            mvwprintw(tabela_produtos, 1, 1+(tabelaX - 124)/2, " ID │   COD INTERNO   │                        NOME                        │   FABRICANTE   │ UN │ QNTD │ VALOR UN │ SUBTOTAL "); 
        else if (tabelaX < 124)
            mvwprintw(tabela_produtos, 1, 1+(tabelaX - 83)/2, " ID │           NOME              │   FABRICANTE   │ QNTD │ VALOR UN │ SUBTOTAL "); 

        if (total_itens > 0)
        {
            int ids_usados[total_itens], j = 0;
            produto P;

            for (int i = 1; i <= maior_id; i++)
            {
                int retorno_select = select_id(&P, i);
         
                if (retorno_select)
                {
                    ids_usados[j] = P.id;
                    j++;
                }
                if (j == total_itens) break;
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
                
                if (P.id_fabrica == NULL || P.nome == NULL || P.fabricante == NULL || P.quantidade == 0 || P.unidade == NULL || P.valor_uni == 0 || P.subtotal == 0)
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
            const int quadrado_removerX = (140+quadrado_removerY)/3;
            const int quadrado_pY = ALTURA_MAX_CONSOLE/4;

            WINDOW * janela_externa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
            WINDOW * janela_remover_produtos = newwin(quadrado_removerY, quadrado_removerX, quadrado_pY, quadrado_pY);
            WINDOW * janela_info = newwin(quadrado_removerY, quadrado_removerX, quadrado_pY, (188*LARGURA_MAX_CONSOLE)/273);
            WINDOW * janela_titulo = newwin(7, 40, 20, (LARGURA_MAX_CONSOLE/2)-20);
            WINDOW * ler_remover = newwin(10, quadrado_removerX-2, quadrado_pY+6, quadrado_pY+1);
            refresh();

            box(janela_externa, 0, 0);
            box(janela_remover_produtos, 0, 0);
            box(janela_info, 0, 0);
            box(janela_titulo, 0, 0);

            mvwprintw(janela_titulo, 3, 11, "REMOVER  PRODUTOS");
            mvwprintw(janela_remover_produtos, quadrado_removerY/10, (quadrado_removerX/2)-8, "Tipos de Remoção");

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
            int tecla_pressionada = 0, opc_selecionada = 0, y_atual = getcury(janela_remover_produtos);

            keypad(janela_remover_produtos, true);
            curs_set(0);
            noecho();

            while (tecla_pressionada != 10)
            {   
                for (int i = 0; i < tipos_remover; i++)
                {
                    if (i == opc_selecionada) wattron(janela_remover_produtos, A_STANDOUT);
                    mvwprintw(janela_remover_produtos, y_atual+4+i*3, 4, lista_remover[i]);
                    wattroff(janela_remover_produtos, A_STANDOUT);
                }
                wrefresh(janela_remover_produtos);

                tecla_pressionada = wgetch(janela_remover_produtos);

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

            if (opc_selecionada == 0)
            {

                curs_set(1);
                wclear(janela_remover_produtos);
                box(janela_remover_produtos, 0, 0);
                mvwprintw(janela_remover_produtos, y_atual, (quadrado_removerX/2)-9, "Remover um Produto");
                wrefresh(janela_remover_produtos);

                mvwprintw(ler_remover, 0, 2, "Digite o ID do produto: ");
                wgetnstr(ler_remover, buffer_rem, 21);
                wrefresh(ler_remover);
                
                int id_rem = atoi(buffer_rem);
                
                if (id_rem <= 0)
                {
                    mvwprintw(ler_remover, 2, 2, "ID inválido, por favor insira outro!");
                    wrefresh(ler_remover);
                    wgetch(ler_remover);
                }
                else
                {
                    mvwprintw(ler_remover, 3, 2, "Você tem certeza que quer");
                    mvwprintw(ler_remover, 4, 2, "apagar o ID %d? [S] / [N]", id_rem);
                    char resposta = mvwgetch(ler_remover, 6, 4);
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
                wclear(janela_remover_produtos);
                box(janela_remover_produtos, 0, 0);
                mvwprintw(janela_remover_produtos, y_atual, (quadrado_removerX/2)-14, "Remover um grupo de Produtos");
                wrefresh(janela_remover_produtos);

                mvwprintw(ler_remover, 0, 2, "Digite o ID inicial: ");
                wgetnstr(ler_remover, buffer_rem, 21);
                wrefresh(ler_remover);
                
                int id_inicio_rem = atoi(buffer_rem);

                if (id_inicio_rem <= 0)
                {
                    mvwprintw(ler_remover, 2, 2, "ID inválido, por favor insira outro!");
                    wrefresh(ler_remover);
                    wgetch(ler_remover);
                }
                else
                {
                    buffer_rem[0] = '\0';
                    mvwprintw(ler_remover, 2, 2, "Digite o ID final: ");
                    wgetnstr(ler_remover, buffer_rem, 21);
                    wrefresh(ler_remover);

                    int id_final_rem = atoi(buffer_rem);

                    if (id_final_rem <= 0 || (id_final_rem < id_inicio_rem))
                    {
                        mvwprintw(ler_remover, 4, 2, "ID inválido, por favor insira outro!");
                        wrefresh(ler_remover);
                        wgetch(ler_remover);
                    }
                    else
                    {
                        mvwprintw(ler_remover, 5, 2, "Você tem certeza que quer");
                        mvwprintw(ler_remover, 6, 2, "apagar todos os IDs de %d até %d? [S] / [N]", id_inicio_rem, id_final_rem);
                        wrefresh(ler_remover);
                        char resposta = mvwgetch(ler_remover, 8, 2);

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

            endwin();
            break;
            return 1;
        }
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
    buffer_sql[0] = '\0';
    for (int i = 0; i < argc; i++)
    {
        strcat(buffer_sql, argv[i]);
        if (i != argc-1) strcat(buffer_sql, ",");
    }
    return 0;
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
#include <pdcurses/curses.h>
#include <windows.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdlib.h>

#define LARGURA_MAX_JANELA GetSystemMetrics(SM_CXMAXIMIZED)
#define ALTURA_MAX_JANELA GetSystemMetrics(SM_CYMAXIMIZED)
#define LARGURA_MAX_CONSOLE getmaxx(stdscr)
#define ALTURA_MAX_CONSOLE getmaxy(stdscr)
#define VERSAO_PROGRAMA 0.1

typedef struct {
    char nome_empresa[51];
    int total_produtos;
} config;

config empresa;

void menu(int x);

int main()
{
    SetWindowPos(GetConsoleWindow(), HWND_NOTOPMOST, 0, 0, LARGURA_MAX_JANELA, ALTURA_MAX_JANELA, SWP_SHOWWINDOW);
    empresa.total_produtos = 0;

    FILE *configs = fopen("Dados_Programa\\configs.txt", "r");
    if (configs == NULL)
    {
        mkdir("Dados_Programa");
        configs = fopen("Dados_Programa\\configs.txt", "w");

        initscr();
        
        WINDOW * janela_nome_empresa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
        WINDOW * inserir_nome_empresa = newwin(3,52, (ALTURA_MAX_CONSOLE-1)/2, (LARGURA_MAX_CONSOLE-52)/2);;
        box(janela_nome_empresa, 0, 0);
        wborder(inserir_nome_empresa, 9553, 9553, 9552, 9552, 9556, 9559, 9562, 9565);

        char *buffer = (char *)malloc(sizeof(char)*51);

        mvwprintw(janela_nome_empresa, ((ALTURA_MAX_CONSOLE-1)/2)-3, ((LARGURA_MAX_CONSOLE-52)/2)+7, "Por favor insira o nome da sua Empresa");
        refresh();
        wrefresh(janela_nome_empresa);
        wrefresh(inserir_nome_empresa);
        mvwgetstr(inserir_nome_empresa, 1, 1, buffer);
        fprintf(configs, buffer);
        fprintf(configs, ",\n");

        strcpy(empresa.nome_empresa, buffer);
        free(buffer);

        delwin(inserir_nome_empresa);
        delwin(janela_nome_empresa);

        endwin();

    }
    else fscanf(configs, "%50[^,]",empresa.nome_empresa);

    fclose(configs);
    menu(1);
	return 0;
}  

void menu(int x)
{
    SetWindowPos(GetConsoleWindow(), HWND_NOTOPMOST, 0, 0, LARGURA_MAX_JANELA, ALTURA_MAX_JANELA, SWP_SHOWWINDOW);
    switch (x)
    {
        case 1:
        {
            initscr();

            int vertical_esquerda, 
            vertical_direita, 
            horizontal_topo, 
            horizontal_baixo, 
            sup_esquerdo, 
            sup_direito, 
            inf_esquerdo, 
            inf_direito;

            noecho();
            cbreak();
            curs_set(0);

                WINDOW * caixa_opcoes_maior = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE/3, 0, 0);
                WINDOW * caixa_opcoes_menor = newwin(ALTURA_MAX_CONSOLE-16, LARGURA_MAX_CONSOLE/3, 10, 0);
                WINDOW * caixaInfo = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE-(LARGURA_MAX_CONSOLE/3), 0, (LARGURA_MAX_CONSOLE/3)-1);

                vertical_esquerda = vertical_direita = horizontal_topo = horizontal_baixo = sup_esquerdo = sup_direito = inf_esquerdo = inf_direito = 0;

                box(caixa_opcoes_maior, horizontal_topo, vertical_esquerda);

                sup_esquerdo = 9516;
                inf_esquerdo = 9524;
                wborder(caixaInfo, vertical_esquerda, vertical_direita, horizontal_topo, horizontal_baixo, sup_esquerdo, sup_direito, inf_esquerdo, inf_direito);

                sup_esquerdo = inf_esquerdo = 9500;
                sup_direito = inf_direito = 9508;
                wborder(caixa_opcoes_menor, vertical_esquerda, vertical_direita, horizontal_topo, horizontal_baixo, sup_esquerdo, sup_direito, inf_esquerdo, inf_direito);

                    mvwprintw(caixa_opcoes_maior, 4, (getmaxx(caixa_opcoes_maior) - 21)/2,"CONTROLE DE ESTOQUE");
                    mvwprintw(caixaInfo, 4, ( getmaxx(caixa_opcoes_maior) - 21)/2, "%s", empresa.nome_empresa);
                    mvwprintw(caixa_opcoes_maior, 6, ( getmaxx(caixa_opcoes_maior) - 4)/2, "%.1f",VERSAO_PROGRAMA);

                    mvwprintw(caixa_opcoes_maior, ALTURA_MAX_CONSOLE - 4, ( getmaxx(caixa_opcoes_maior) - 30)/2, "[  DATA:  04 / 11 / 2024  ]");

                    refresh();
                    wrefresh(caixa_opcoes_maior);
                    wrefresh(caixa_opcoes_menor);
                    wrefresh(caixaInfo);

                char menu_principal_opcoes[6][30] =
                {
                    {"Adicionar Produto"},
                    {"Editar Produto"},
                    {"Remover Produto"},
                    {"Mostrar todos os Produtos"},
                    {"Mostrar com Filtros"},
                    {"Sair"}
                };

                keypad(caixa_opcoes_menor, true);

                int opcao_selecionada = 0, tecla_pressionada;

                while (tecla_pressionada != 10)
                {

                    for (int i = 0; i < 6; i++)
                    {
                        if (i == opcao_selecionada) wattron(caixa_opcoes_menor, A_STANDOUT);
                        mvwprintw( caixa_opcoes_menor, (getmaxy(caixa_opcoes_menor) / 7 ) + ( i*2 ), getmaxx(caixa_opcoes_menor) / 6, "%s", menu_principal_opcoes[i] );
                        wattroff(caixa_opcoes_menor, A_STANDOUT);  
                    }

                    tecla_pressionada = wgetch(caixa_opcoes_menor);

                    switch (tecla_pressionada)
                    {
                        case KEY_DOWN:
                            opcao_selecionada++;
                            if (opcao_selecionada > 5) opcao_selecionada = 0;
                            break;
                        case KEY_UP:
                            opcao_selecionada--;
                            if (opcao_selecionada < 0) opcao_selecionada = 5;
                            break;
                    }

                }

            endwin(); 
        break;
        }
        default:
            break;
    }
}

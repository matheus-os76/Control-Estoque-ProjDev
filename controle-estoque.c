#include <pdcurses/curses.h>
#include <windows.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>

#define LARGURA_MAX_JANELA GetSystemMetrics(SM_CXMAXIMIZED)
#define ALTURA_MAX_JANELA GetSystemMetrics(SM_CYMAXIMIZED)
#define LARGURA_MAX_CONSOLE getmaxx(stdscr)
#define ALTURA_MAX_CONSOLE getmaxy(stdscr)
#define VERSAO_PROGRAMA 0.1

char nome_empresa[51];
int total_produtos = 0;

void menu(int x, struct tm *y);

int main()
{
    time_t tempo_atual = time(NULL);
    struct tm *data = localtime(&tempo_atual);

    SetWindowPos(GetConsoleWindow(), HWND_NOTOPMOST, 0, 0, LARGURA_MAX_JANELA, ALTURA_MAX_JANELA, SWP_SHOWWINDOW);

    FILE *configs = fopen("Dados_Programa\\configs.txt", "r");

        if (configs == NULL)
        {
            mkdir("Dados_Programa");
            configs = fopen("Dados_Programa\\configs.txt", "w");

            initscr();
            
                WINDOW * janela_nome_empresa = newwin(ALTURA_MAX_CONSOLE, LARGURA_MAX_CONSOLE, 0, 0);
                WINDOW * inserir_nome_empresa = newwin(3, 52, ( ALTURA_MAX_CONSOLE - 1 ) / 2, ( LARGURA_MAX_CONSOLE - 52 ) / 2);

                box(janela_nome_empresa, 0, 0);
                wborder(inserir_nome_empresa, 9553, 9553, 9552, 9552, 9556, 9559, 9562, 9565);

                char *buffer = (char *)malloc(sizeof(char)*51);

                    mvwprintw(janela_nome_empresa, ( ( ALTURA_MAX_CONSOLE - 1 ) / 2 ) - 3, ( ( LARGURA_MAX_CONSOLE - 52 ) / 2 ) + 7, "Por favor insira o nome da sua Empresa");
                    refresh();
                    wrefresh(janela_nome_empresa);
                    wrefresh(inserir_nome_empresa);
                    
                    mvwgetstr(inserir_nome_empresa, 1, 1, buffer);
                    strcpy(nome_empresa, buffer);

                    fprintf(configs, buffer);
                    fprintf(configs, ",\n");
                
                free(buffer);
                
                delwin(inserir_nome_empresa);
                delwin(janela_nome_empresa);

            endwin();

        }
        else fscanf(configs, "%50[^,]",nome_empresa);

    fclose(configs);
    menu(1, data);
	return 0;
}  

void menu(int x, struct tm *y)
{

    switch (x)
    {
        case 1:
        {
            initscr();

            noecho();
            cbreak();
            curs_set(0);

                const int opcaoMaiorY = ALTURA_MAX_CONSOLE;
                const int opcaoMaiorX = LARGURA_MAX_CONSOLE/3;

                const int opcaoMenorY = ALTURA_MAX_CONSOLE-16;

                const int InfoX = LARGURA_MAX_CONSOLE-(opcaoMaiorX);

                WINDOW * caixa_opcoes_maior = newwin( opcaoMaiorY, opcaoMaiorX, 0, 0 );
                WINDOW * caixa_opcoes_menor = newwin( opcaoMenorY, opcaoMaiorX, 10, 0 );
                WINDOW * caixaInfo = newwin( opcaoMaiorY, InfoX, 0, opcaoMaiorX - 1 );

                box(caixa_opcoes_maior, 0, 0);
                wborder(caixaInfo, 0, 0, 0, 0, 9516, 0, 9524, 0);
                wborder(caixa_opcoes_menor, 0, 0, 0, 0, 9500, 9508, 9500, 9508);

                    mvwprintw(caixa_opcoes_maior, 4, ( opcaoMaiorX - 21 ) / 2,
                        "CONTROLE DE ESTOQUE");

                    mvwprintw(caixa_opcoes_maior, 6, ( opcaoMaiorX - 4 ) / 2, 
                        "%.1f", VERSAO_PROGRAMA);

                    mvwprintw(caixa_opcoes_maior, ( opcaoMaiorY - 4 ), ( opcaoMaiorX - 30 ) / 2, 
                        "[  DATA: %2d  /  %2d  /  %4d  ]",
                        y->tm_mday, y->tm_mon+1, y->tm_year+1900);

                    mvwprintw(caixaInfo, 4, ( InfoX - strlen(nome_empresa) ) / 2, 
                        "%s", nome_empresa);

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
                delwin(caixa_opcoes_maior);
                delwin(caixa_opcoes_menor);
                delwin(caixaInfo);

            endwin(); 
        break;
        }
        case 2:
        {
            break;
        }
    }
}

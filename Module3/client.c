#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 4096
#define TRUE 1

volatile sig_atomic_t flag = 0; // Variável de sinalização para indicar se o programa deve sair
int sockfd = 0; // Descritor do socket para comunicação com o servidor
char name[30]; // Armazena o nome do usuário

// Função auxiliar para limpar e formatar o buffer de saída padrão
void format_stdout() {
	printf("%s", "> ");
	fflush(stdout);
}

// Função auxiliar para remover a nova linha de uma string
void remove_line(char *arr, int length) {
	for (int i = 0; i < length; i++) {
		if (arr[i] == '\n') {
			arr[i] = '\0';
			break;
		}
	}
}

// Manipulador de sinal para capturar SIGINT (Ctrl+C) e definir a flag para sair
void catch_ctrl_c_and_exit(int sig) {
	flag = 1;
}

// Função executada em uma thread separada para lidar com o envio de mensagens
void send_msg() {
	char message[LENGTH] = {};	// Recebe a mensagem
	char buffer[LENGTH + 32] = {};	// Envia a mensagem

	while (TRUE) {
		format_stdout();	// Formação para o chat
		fgets(message, LENGTH, stdin); // Recebe a mensagem
		remove_line(message, LENGTH); // Remove o pula linha

		if (strcmp(message, "/quit") == 0) { // Lida com a saída
			break;
		}
		else if (strstr(message, "/nickname") != NULL) {
			char *nick = strtok(message, " ");
			nick = strtok(NULL, " ");

			if (nick != NULL) {
        		strcpy(name, nick); // Copy the second word to cli->name
    		}
		}
		else {
			sprintf(buffer, "%s: %s\n", name, message); // Envia a mensagem formatada para o servidor Name: Mensagem
			send(sockfd, buffer, strlen(buffer), 0); // Envia a mensagem 
		}

		// Limpa a memória para as próximas strings
		bzero(message, LENGTH);	
		bzero(buffer, LENGTH + 32); 
	}

	catch_ctrl_c_and_exit(2); // Lida com o Ctrl+C
}

// Função executada em uma tread separada para lidar com o recebimento de mensagens
void recv_msg() {
	char message[LENGTH] = {}; // Guarda a mensagem

	// Enquanto houve o recebimento de mensagem, faz o recebimento
	while (TRUE) {
		int receive = recv(sockfd, message, LENGTH, 0); // Recebe a mensagem

		if (receive > 0) { // Se existe, imprime
			printf("%s", message); // Imprime a mensagem
			format_stdout(); // Formata o chat

			if (strstr(message, "/kick") && strstr(message, name)) {
				catch_ctrl_c_and_exit(2); // Lida com o Ctrl+C
			}

		}
		else if (receive == 0) { // Senão, não
			break;
		}
	
		memset(message, 0, sizeof(message)); // Limpa a memória da mensagem
	}
}

// Função executada para interagir com a entrada do usuário
int init() {
	char input[100]; // Recebe os comandos

    printf("Enter /connect to start:\n> ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0'; 

	// Se for '/connect' conecta ao chat
    if (strcmp(input, "/connect") == 0) {
        printf("\nConnection established!\n\n");
		return 1;
    } else {
		return 0;
	}
}

int main(int argc, char **argv) {

	while (!init()) // Função de interação inicial

	if (argc != 2) { // Verifica se o número de argumentos na linha de comando está correto
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1"; // Endereço IP do servidor (Loopback)
	int port = atoi(argv[1]); // Porta fornecida como argumento da linha de comando

	signal(SIGINT, catch_ctrl_c_and_exit); // Configura o manipulador de sinal para capturar o SIGINT (Ctrl+C)

	printf("Please enter your name:\n> ");
	fgets(name, 30, stdin);
	remove_line(name, strlen(name));

	if (strlen(name) > 30 || strlen(name) < 2) { // Verifica se o nome do usuário é válido
		printf("Name must be less than 28 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr; // Estrutura para armazenar informações sobre o servidor

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Cria um socket TCP
	server_addr.sin_family = AF_INET; // Configura a família de endereços do socket como IPv4
	server_addr.sin_addr.s_addr = inet_addr(ip); // Configura o endereço IP do servidor
	server_addr.sin_port = htons(port); // Configura a porta do servidor

	// Estabelece uma conexão com o servidor
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

	if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	send(sockfd, name, 30, 0); // Envia o nome do usuário para o servidor

	printf("\n=== WELCOME TO THE CHATROOM ===\n");

	pthread_t sendMsgThread; // Thread para lidar com o envio de mensagens do cliente

	if (pthread_create(&sendMsgThread, NULL, (void *)send_msg, NULL) != 0) {
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t recvMsgThread; // Thread para lidar com a recepção de mensagens do servidor

	if (pthread_create(&recvMsgThread, NULL, (void *)recv_msg, NULL) != 0) {
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (TRUE) { // Loop principal para aguardar a sinalização de saída
		if (flag) {
			printf("\nBye\n");
			break;
		}
	}

	close(sockfd); // Fecha o socket

	return EXIT_SUCCESS;
}
//SSOO-P3 2022-2023

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#define MAX_NUM_OP 200

/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */

int max_cuentas = 0;
int n_operation = 0; // Number of operations on file.txt
int client_numop = 0; // Increments every time a cliente wants to make an operation from cajero
int bank_numop = 0; // Increments every time a trabajador wants to make an operation from buffer
int global_balance = 0; // Updated with operations executed
int n_accounts = 0;
struct account **account_list = NULL;
queue *q;

pthread_cond_t no_vacio;
pthread_cond_t no_lleno;
pthread_mutex_t mutex_trabajadores;

struct account {
    int id;
    int balance;
};

int crear(int id) {
    if (n_accounts >= max_cuentas) {
        fprintf(stderr, "[ERROR]: Reached maximum number of accounts (%d)\n", max_cuentas);
        return -1;
    }
    n_accounts++;
    account_list = (struct account**) realloc(account_list, sizeof(struct account*) * n_accounts);
    if (account_list == NULL) {
        fprintf(stderr, "[ERROR]: Memory allocation error creating account\n");
        return -1;
    }
    struct account* ans = (struct account*) malloc(sizeof (struct account));
    account_list[n_accounts - 1] = ans;
    ans -> id = id;
    return 0;
}

int find_account(int account_id) {
    int i = 0;
    int found = -1;
    while(found < 0 && i < n_accounts) {
        if (account_list[i] -> id == account_id) {
            found = i;
        }
        i++;
    }
    if (found == -1) {
        fprintf(stderr, "[ERROR]: Account %d does not exist\n", account_id);
        return -1;
    }
    return found;
}

int ingresar(int account_id, int to_add) {
    int err = find_account(account_id);
    if (err < 0) {
        fprintf(stderr, "[ERROR]: In deposit (ingresar) operation\n");
        return -1;
    }
    account_list[err] -> balance += to_add;
    global_balance += to_add;
    return 0;
}

int retirar(int account_id, int to_substract) {
    if (to_substract < 0) {
        fprintf(stderr, "[ERROR]: Insufficient balance for withdrawal\n");
        return -1;
    };
    int err = find_account(account_id);
    if (err < 0) {
        fprintf(stderr, "[ERROR]: In withdraw (retirar) operation\n");
        return -1;
    }
    account_list[err] -> balance -= to_substract;
    global_balance -= to_substract;
    return 0;
}


int traspasar(int emitting_account_id, int receiving_account_id, int to_transfer) {
    if (to_transfer < 0) {
        fprintf(stderr, "[ERROR]: You cannot transfer negative amounts of money.\n");
        return -1;
    }
    int err = find_account(emitting_account_id);
    if (err < 0) {
        fprintf(stderr, "[ERROR]: In transfer (traspasar) operation\n");
        return -1;
    }
    if (to_transfer > account_list[err]-> balance) {
        fprintf(stderr, "[ERROR]: Insufficient balance for transfer\n");
        return -1;
    }
    retirar(emitting_account_id,to_transfer);
    ingresar(receiving_account_id, to_transfer);
    return 0;
}

int saldo(int account_id) {
    int err = find_account(account_id);
    if (err < 0) {
        fprintf(stderr, "[ERROR]: In balance (saldo) operation\n");
        return -1;
    }
    return account_list[err] -> balance;
}

int* extract_arguments(char* operation, int n_arguments, int str_pos)
{
    int *arguments = malloc(n_arguments);
    int arguments_it = 0;
    str_pos++;
    while (str_pos < strlen(operation))
    {
        char integer_argument[10]; // maximum size of arguments [2]
        int iterator_integer_argument = 0;
        while (str_pos < strlen(operation) && operation[str_pos] != ' ')
        {
            if (iterator_integer_argument > 9)
            {
                fprintf(stderr, "[ERROR]: Maximum argument size: 9 digits\n");
                exit(-1);
            }
            integer_argument[iterator_integer_argument] = operation[str_pos];
            iterator_integer_argument++;
            str_pos++;
        }
        if (operation[str_pos] == '\0' || operation[str_pos] == ' ')
        {
            integer_argument[iterator_integer_argument] = '\0';
            arguments[arguments_it] = atoi(integer_argument);
            arguments_it++;
        }
        str_pos++;
    }
    return arguments;
}

int execute_operation(struct element* element)
{
    int saldo_arg = 0;
    char* operation = element -> operation;
    char* command = malloc(1);
    int* arguments;
    int i = 0;
    // We get operation command to execute
    while (operation[i] != ' ')
    {
        command = realloc(command, sizeof(char) * (i+1));
        command[i] = operation[i];
        i++;
    }
    if (strcmp(command, "CREAR") == 0)
    {
        arguments = extract_arguments(operation, 1, i);
        int err = crear(arguments[0]);
        if (err < 0)
        {
            fprintf(stderr, "[ERROR]: Cannot create account\n");
            return -1;
        }
    }
    else if (strcmp(command, "INGRESAR") == 0)
    {
        arguments = extract_arguments(operation, 2, i);
        int err = ingresar(arguments[0], arguments[1]);
        if (err < 0)
        {
            fprintf(stderr, "[ERROR]: Cannot deposit into account\n");
            return -1;
        }
    }
    else if (strcmp(command, "RETIRAR") == 0)
    {
        arguments = extract_arguments(operation, 2, i);
        int err = retirar(arguments[0], arguments[1]);
        if (err < 0)
        {
            fprintf(stderr, "[ERROR]: Cannot withdraw from account\n");
            return -1;
        }
    }
    else if (strcmp(command, "TRASPASAR") == 0)
    {
        saldo_arg = 1;
        arguments = extract_arguments(operation, 3, i);
        int err = traspasar(arguments[0], arguments[1], arguments[2]);
        if (err < 0)
        {
            fprintf(stderr, "[ERROR]: Cannot transfer between accounts\n");
            return -1;
        }
    }
    else if (strcmp(command, "SALDO") == 0)
    {
        arguments = extract_arguments(operation, 1, i);
        int err = saldo(arguments[0]);
        if (err < 0)
        {
            fprintf(stderr, "[ERROR]: Cannot show saldo from account\n");
            return -1;
        }
    }
    else
    {
        fprintf(stderr, "[ERROR]: Operation %s is not valid\n", command);
        return -1;
    }
    printf("%d %s SALDO=%d TOTAL=%d\n", bank_numop + 1, element->operation, saldo(arguments[saldo_arg]), global_balance);
    return 0;
}


void *Trabajador(void *arg) {
    element *dequeued;
    while(bank_numop < n_operation) {
        pthread_mutex_lock(&mutex_trabajadores);
        //if(bank_numop == n_operation) {
        //    pthread_cond_signal(&no_vacio);
        //    pthread_mutex_unlock(&mutex_trabajadores);
        //    pthread_exit(0);
        //}
        while (queue_empty(q) && bank_numop < n_operation) {
            pthread_cond_wait(&no_vacio, &mutex_trabajadores);
        }
        if(bank_numop == n_operation) {
            pthread_cond_signal(&no_vacio);
            pthread_mutex_unlock(&mutex_trabajadores);
            pthread_exit(0);
        }
        // SECCION CRITICA
        dequeued = queue_get(q); // Extraemos de la cola circular
        if (dequeued == NULL) {
            fprintf(stderr, "[ERROR] Dequeue operation not valid in Cajero thread\n");
            pthread_exit(0);
        }
        execute_operation(dequeued);
        bank_numop++; // Incrementamos variable bank_numop
        // FIN SECCION CRITICA
        pthread_cond_signal(&no_lleno);
        pthread_mutex_unlock(&mutex_trabajadores);
    }
    pthread_cond_signal(&no_vacio);
    pthread_mutex_unlock(&mutex_trabajadores);
    pthread_exit(0);
}


void *Cajero(void *arg) {
    element **list_client_ops = arg;
    while(client_numop < n_operation) {
        pthread_mutex_lock(&mutex_trabajadores);
        while (queue_full(q) && client_numop < n_operation) {
            pthread_cond_wait(&no_lleno, &mutex_trabajadores);
        }
        if(client_numop == n_operation) {
            pthread_cond_broadcast(&no_lleno);
            pthread_mutex_unlock(&mutex_trabajadores);
            pthread_exit(0);
        }
        // SECCION CRITICA
        element *to_enqueue = list_client_ops[client_numop];
        int err = queue_put(q, to_enqueue);
        if(err < 0){
            fprintf(stderr, "[ERROR] Queue operation not valid in Cajero thread\n");
            pthread_exit(0);
        }
        client_numop++;
        // FIN SECCION CRITICA
        pthread_cond_signal(&no_vacio);
        pthread_mutex_unlock(&mutex_trabajadores);
    }
    pthread_exit(0);
}

int validate_operation (char * operation_line) {
    int i = 0;
    while (operation_line[i] != '\0') {
        if (!isdigit(operation_line[i])) {
            fprintf(stderr, "[ERROR]: operation file does not contain number operations as a header\n");
            return -1;
        }
        i++;
    }
    return 0;
}

struct element** extract_operations(const char *input_file_path) {
    // Open the file and error handling
    int fd = open(input_file_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[FATAL ERROR] File cannot be read\n");
        fprintf(stderr, "Exiting the program\n");
        exit(-1);
    }

    int nread = 0;
    struct element **to_return;
    int size_operation_string = 0;
    char buffer[1];
    int element_index = 0;
    char *operation_line = malloc(1); // String to store operation(full line)

    while ((nread = read(fd, buffer, 1)) > 0) {
        // Dynamic allocation of operation_line
        if (*buffer != '\n') {
            operation_line = realloc(operation_line, sizeof(char) * (size_operation_string + 1));
            operation_line[size_operation_string] = *buffer;
            size_operation_string++;
        }
        // AT THE END OF THE LINE
        if (*buffer == '\n' || *buffer == '\0') {
            if (n_operation == 0) {
                /* If the n_operation has not been set, we set it (as it should be the first line) */
                int err = validate_operation(operation_line);
                if(err < 0) {
                    fprintf(stderr, "[FATAL ERROR] extract_operations, cannot validate operation: %s\n", operation_line);
                    fprintf(stderr, "Exiting the program...\n");
                    exit(-1);
                }
                n_operation = atoi(operation_line);
                if (n_operation > MAX_NUM_OP) {
                    fprintf(stderr, "[FATAL ERROR] Cannot read from the file\n");
                    fprintf(stderr, "Exiting the program...\n");
                    exit(-1);
                }
                to_return = malloc(sizeof(element *) * n_operation +1);
            } else {
                /* If the n_operation has been set, we create an element with the operation_line as its argument,
                   and we save it in the to_return vector */
                operation_line = realloc(operation_line, sizeof(char) * (size_operation_string + 1));
                operation_line[size_operation_string] = '\0';
                to_return[element_index] = element_init(operation_line);
                element_index++;
            }
            // New for the new operation_line string, reset size to 0
            operation_line = malloc(1);
            size_operation_string = 0;
        }
    }

    // Last operation (\n)
    operation_line = realloc(operation_line, sizeof(char) * (size_operation_string + 1));
    operation_line[size_operation_string] = '\0';
    to_return[element_index] = element_init(operation_line);
    element_index++;

    if (element_index != n_operation) {
        close(fd);
       fprintf(stderr, "[FATAL ERROR] Number of operations different from indicated in file (%d)\n", n_operation);
       fprintf(stderr, "Exiting the program...\n");
       exit(-1);
    }

    // Read error handling
    if (nread < 0) {
        close(fd);
        fprintf(stderr, "[FATAL ERROR] Cannot read from the file\n");
        fprintf(stderr, "Exiting the program...\n");
        exit(-1);
    }
    close(fd);
    return to_return;
}



int main (int argc, const char * argv[] ) {

    /* Argument processing and validation*/
    if (argc != 6) {
        fprintf(stderr, "[ERROR] Usage: ./bank <nombre_fichero> <num_cajero> <max_cuentas> <tam_buff>\n");
        return -1;
    }
    struct element** list_client_ops = extract_operations(argv[1]); // Exception Handling NEEDED

    int n_cajeros = atoi(argv[2]);
    if (n_cajeros <= 0 || n_cajeros >= 10000) {
        fprintf(stderr, "[ERROR] Number of cashiers (cajeros) must be between 1 and 10000\n");
        return -1;
    }

    int n_trabajadores = atoi(argv[3]);
    if (n_trabajadores <= 0 || n_trabajadores >= 10000) {
        fprintf(stderr, "[ERROR] Number of workers (trabajadores) must be between 1 and 10000\n");
        return -1;
    }

    max_cuentas = atoi(argv[4]);
    if (max_cuentas <= 0 || max_cuentas >= 10000) {
        fprintf(stderr, "[ERROR] Number of maximum accounts (cuentas) must be between 1 and 10000\n");
        return -1;
    }

    int size = atoi(argv[5]);
    if (size <= 0 || size >= 200) {
        fprintf(stderr, "[ERROR] Size of queue (buffer) must be between 1 and 200\n");
        return -1;
    }

    /* INITIALIZATION OF NEEDED VARIABLES*/
    account_list = (struct account **) malloc(1);
    if (account_list == NULL) {
        fprintf(stderr, "[ERROR]: Memory allocation error creating account_list\n");
        return -1;
    }
    pthread_cond_init(&no_vacio, NULL); // Exception Handling NEEDED kill
    pthread_cond_init(&no_lleno, NULL); // Exception Handling NEEDED kill
    pthread_mutex_init(&mutex_trabajadores, NULL); // Exception Handling NEEDED kill
    q = queue_init(size); // Exception Handling NEEDED

    pthread_t **list_of_cajero_threads = malloc(sizeof (pthread_t*) * n_cajeros);
    if (list_of_cajero_threads == NULL) {
        fprintf(stderr, "[ERROR]: Memory allocation error creating list_of_cajero_threads\n");
        return -1;
    }

    pthread_t **list_of_work_threads = malloc(sizeof (pthread_t*) * n_trabajadores);
    if (list_of_work_threads == NULL) {
        fprintf(stderr, "[ERROR]: Memory allocation error creating list_of_work_threads\n");
        return -1;
    }

    /* INITIALIZATION OF CAJERO THREADS */
    for (int i = 0; i < n_cajeros; i++) {
        list_of_cajero_threads[i] = (pthread_t *)malloc(sizeof (pthread_t*));
        if (list_of_cajero_threads[i] == NULL) {
            fprintf(stderr, "[ERROR]: Memory allocation error creating list_of_cajero_threads[%d]\n", i);
            return -1;
        }
        int rc = pthread_create(list_of_cajero_threads[i], NULL, Cajero, list_client_ops);
        if (rc) {
            fprintf(stderr, "Error creating thread: %d\n", rc);
            return -1;
        }
    }

    /* INITIALIZATION OF TRAJADORES THREADS */
    for (int i = 0; i < n_trabajadores; i++) {
        list_of_work_threads[i] = (pthread_t *)malloc(sizeof (pthread_t*));
        if (list_of_work_threads[i] == NULL) {
            fprintf(stderr, "[ERROR]: Memory allocation error creating list_of_work_threads[%d]\n", i);
            return -1;
        }
        int rc = pthread_create(list_of_work_threads[i], NULL, Trabajador, NULL);
        if (rc) {
            fprintf(stderr, "Error creating thread: %d\n", rc);
            return -1;
        }
    }

    /* JOIN OF ALL THREADS */
    for (int i = 0; i < n_cajeros; i++) {
        int rc = pthread_join(*list_of_cajero_threads[i], NULL);
        if (rc) {
            fprintf(stderr, "Error joining thread: %d\n", rc);
            return -1;
        }
    }

    for (int i = 0; i < n_trabajadores; i++) {
        int rc = pthread_join(*list_of_work_threads[i], NULL);
        if (rc) {
            fprintf(stderr, "Error joining thread: %d\n", rc);
            return -1;
        }
    }

    /* DEALLOCATION OF MEMORY */
    pthread_mutex_destroy(&mutex_trabajadores);
    pthread_cond_destroy(&no_vacio);
    pthread_cond_destroy(&no_lleno);

    return 0;
}

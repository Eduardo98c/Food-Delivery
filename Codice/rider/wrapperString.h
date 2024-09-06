#include<stdio.h>	
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include<string.h>
#include<time.h>
#include<errno.h>
#include<assert.h> 


//Splitta una stringa in base ad un delimitatore in più stringhe
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Conta quanti elementi verranno estratti */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Aggiunge lo spazio per il token finale */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Aggiunge lo spazio per terminare una stringa nulla in modo che il
       chiamante sappia dove finisce l'elenco delle stringhe restituite */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

//Rimuove tutte le occorrenze di un determinato carattere
char* remove_all_chars(char* s, char *c) {
    int k, dim_chars_delete = strlen(c);
    int j, n = strlen(s);
    for(k = 0; k < dim_chars_delete; k++){
       for (int i = j = 0; i < n; i++)
          if (s[i] != c[k])
             s[j++] = s[i];
            
       s[j] = '\0';
    }
    return s;
}

int isNumeric(char *str) 
{
    while(*str != '\0')
    {
        if(*str < '0' || *str > '9')
            return 0;
        str++;
    }
    return 1;
}

/* reverse:  reverse string s in place */
void reverse_string(char s[])
{
   int i, j;
   char c;

   for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
      c = s[i];
      s[i] = s[j];
      s[j] = c;
   }
}  


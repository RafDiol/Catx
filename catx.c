#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<inttypes.h>

#define BUFFER_SIZE 4096

#define SFLAG (1 << 0)
#define LFLAG (1 << 1)

void show_help_message(){
    printf("Catex - An extended CAT util\n");
}

int catextended(uint16_t flags, char* filepath){
    FILE* handler = fopen(filepath, "r");
    if(handler == NULL){
        perror(handler);
        return 2;
    }

    char buffer[BUFFER_SIZE];
    char outputBuffer[BUFFER_SIZE];
    size_t bytesRead = 0; 

    char curr = 0;
    char prev = 0;
    int lineCounter = 1;

    while((bytesRead = fread(buffer, sizeof(char), BUFFER_SIZE, handler))){
        int outputPos = 0;
        for(size_t i = 0; i < bytesRead; i++){
            curr = buffer[i];

            if((flags & SFLAG) && curr == '\n' && (prev == '\n' || prev == 0)){
                prev = curr;
                continue;
            }

            if((flags & LFLAG) && (prev == '\n' || prev == 0)){
                char numStr[20] = {'\0'};
                sprintf(numStr, "%" PRIu16, lineCounter++);
                for(int j = 0; j < 20; j++){
                    if(numStr[j] == '\0'){
                        break;
                    }
                    outputBuffer[outputPos++] = numStr[j];
                }
                outputBuffer[outputPos++] = '\t';
            }   
            
            outputBuffer[outputPos++] = curr;
            prev = curr;

            if(outputPos + 21 > BUFFER_SIZE){ // 21 bytes is the max new buffer increasy
                fwrite(outputBuffer, sizeof(char), outputPos, stdout);
                outputPos = 0;
            }
        }
        if(outputPos > 0){
            fwrite(outputBuffer, sizeof(char), outputPos, stdout);
        }
    }

    return 0;
}

int main(int argc, char* argv[]){
    if(argc < 2){
        show_help_message();
        return 0;
    }
    else if(argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)){
        show_help_message();
    }

    int opt, err;
    uint16_t flags = 0;
    while((opt = getopt(argc, argv, "sl")) != -1){
        switch (opt)
        {
        case 's':
            flags |= SFLAG;
            break;
        case 'l':
            flags |= LFLAG;
            break;
        default:
            return 1;
            break;
        }
    }

    for(int i = optind; i < argc && argv[i][0] != '-'; i++){
        err = catextended(flags, argv[i]);
        if(err != 0){
            return err;
        }
    }

    return 0;
}
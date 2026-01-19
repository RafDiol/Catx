#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<inttypes.h>
#include<sys/ioctl.h>
#include<errno.h>
#include<termios.h>

#define BUFFER_SIZE 4096

#define SQUEEZE_BLANK_FLAG (1 << 0)
#define LINES_FLAG (1 << 1)
#define SHOW_END_FLAG (1 << 2)
#define HIGHLIGHT_FLAG (1 << 3)
#define PAGINATED_FLAG (1 << 4)
// From 1 << 5 to 1 << 13 is paginated value

struct termios term;

double sStrToDouble(char* str, short* flag){
    errno = 0;
    char* endptr;
    double value = strtod(str, &endptr);
    if(endptr == str || errno == ERANGE || *endptr != '\0'){
        *flag = 1;
        errno = 0;
    }
    return value;
}

int get_terminal_rows(void) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        return ws.ws_row;
    }

    return 24; // fallback
}

void term_setup(struct termios *saved) {
    struct termios t;
    tcgetattr(STDIN_FILENO, saved);
    t = *saved;
    t.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void term_restore(const struct termios *saved) {
    tcsetattr(STDIN_FILENO, TCSANOW, saved);
}

void store_Uint8InFlags(uint64_t *flags, uint8_t value, int index) {
    // Clear the 8 bits at position 'index'
    *flags &= ~(0xFFULL << index);

    // Set new bits
    *flags |= ((uint64_t)value) << index;
}

uint8_t read_Uint8InFlags(uint64_t flags, int index){
    return (flags >> index) & 0xFF;
}

void show_help_message(){
    printf("Catex - An extended CAT util\n");
}

int catextended(uint64_t flags, char* filepath){
    FILE* handler = fopen(filepath, "r");
    if(handler == NULL){
        // TODO: Properly handle errors
        perror(handler);
        return 2;
    }

    char buffer[BUFFER_SIZE];
    char outputBuffer[BUFFER_SIZE];
    size_t bytesRead = 0; 

    char curr = 0;
    char prev = 0;
    int lineCounter = 0;
    uint8_t pageLimit = (flags & PAGINATED_FLAG) ? read_Uint8InFlags(flags, 5) : 0;
    int currentPage = 0;

    if(flags & PAGINATED_FLAG){
        term_setup(&term);
    }

    while((bytesRead = fread(buffer, sizeof(char), BUFFER_SIZE, handler))){
        int outputPos = 0;
        for(size_t i = 0; i < bytesRead; i++){
            curr = buffer[i];

            if(curr == '\n'){
                lineCounter++;
            }

            if((flags & SQUEEZE_BLANK_FLAG) && curr == '\n' && (prev == '\n' || prev == 0)){
                prev = curr;
                continue;
            }

            if((flags & LINES_FLAG) && (prev == '\n' || prev == 0)){
                char numStr[20] = {'\0'};
                sprintf(numStr, "%" PRIu16, lineCounter + 1);
                for(int j = 0; j < 20; j++){
                    if(numStr[j] == '\0'){
                        break;
                    }
                    outputBuffer[outputPos++] = numStr[j];
                }
                outputBuffer[outputPos++] = '\t';
            }

            if((flags & SHOW_END_FLAG) && (curr == '\n' || curr == EOF)){
                outputBuffer[outputPos++] = '$';
            }

            if((flags & HIGHLIGHT_FLAG)){
                if(curr == ' '){
                    curr = '.';
                }
                else if(curr == '\t'){
                    curr = '>';
                }
            }
            
            outputBuffer[outputPos++] = curr;
            prev = curr;

            if((flags & PAGINATED_FLAG) && (lineCounter-currentPage*pageLimit) >= pageLimit){
                fwrite(outputBuffer, sizeof(char), outputPos, stdout);
                outputPos = 0;
                currentPage++;
                fprintf(stderr, "--More--\r");
                getchar();
                fprintf(stderr, "\033[2K\r");
                fflush(stderr);
            }
            else if(outputPos + 21 > BUFFER_SIZE){ // 21 bytes is the max new buffer increasy
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
    uint64_t flags = 0;
    while((opt = getopt(argc, argv, "slehpP:")) != -1){
        switch (opt)
        {
        case 's':
            flags |= SQUEEZE_BLANK_FLAG;
            break;
        case 'l':
            flags |= LINES_FLAG;
            break;
        case 'e':
            flags |= SHOW_END_FLAG;
            break;
        case 'h':
            flags |= HIGHLIGHT_FLAG;
            break;
        case 'p':
            flags |= PAGINATED_FLAG;
            store_Uint8InFlags(&flags, get_terminal_rows()-1, 5); // Default paginated value 24
            break;
        case 'P':
            flags |= PAGINATED_FLAG;
            // paginated value from 1 << 5 to 1 << 13
            short f = 0;
            uint8_t paginationValue = (uint8_t)sStrToDouble(optarg, &f);
            if(f != 0){
                fprintf(stderr, "-P <value> is invalid\n");
                return 1;
                break;
            }
            store_Uint8InFlags(&flags, paginationValue, 5);
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

    if(flags & PAGINATED_FLAG){
        term_restore(&term);
    }

    return 0;
}
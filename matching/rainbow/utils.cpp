#include "utils.h"
#include "dictionary.h"
#include <sys/time.h>
#include <dirent.h>


extern unsigned long g_scan;
extern clock_t spend;
extern unsigned int g_match;
extern unsigned long long compress;
extern unsigned long long total;
extern int literal_num;

void GetDictionaryState(short *DictionaryState, const FSM *fsm) {
    const unsigned char *byte = kBrotliDictionaryData;
    short state = 0;
    for (int i = 0; i < DICTIONARY_BYTES; i++) {
        state = fsm->list[state * 256 + *byte];
        *DictionaryState = state;
        byte++;
        DictionaryState++;
    }
}

void Performance() {
    printf("time:%d ms, matched:%d, scan:%d, total:%d, skip-ratio: %.2f%, throughput %.2f mbps,literal-num:%d, pointer-ratio:%.2f\n",
           spend, g_match, g_scan, total, 100 * (1.0 - (g_scan * 1.0 / total)),
           (compress * 8) / (double) (spend) / 1000, literal_num, 100 * (1.0 - (literal_num * 1.0 / total)));
}

int readFileName(const char *path, vector<string> &names) {
    DIR *dir = opendir(path);
    if (!dir) {
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip "." and ".."
        }

        string name = path;
        name += "/";
        name += entry->d_name;

        names.push_back(name);
    }

    closedir(dir);
    return 0;
}

int LoadText(char *path, vector<Content> &buff, vector<string> &name) {
    struct stat statbuf;
    char szDir[256] = {0};
    strcpy(szDir, path);
    DIR *pDir = opendir(szDir);
    struct dirent *entry;
    char szFile[256];
    while ((entry = readdir(pDir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        sprintf(szFile, "%s/%s", szDir, entry->d_name);
        unsigned long dwRead = 0;
        stat(szFile, &statbuf);
        unsigned char *pBuf = (unsigned char *) malloc(statbuf.st_size);
        FILE *hFile = fopen(szFile, "rb");
        if (hFile == NULL) {
            printf("open file %s error\n", entry->d_name);
            continue;
        }
        fread(pBuf, statbuf.st_size, 1, hFile);
        fclose(hFile);
        buff.push_back({pBuf, static_cast<size_t>(statbuf.st_size)});
    }
    closedir(pDir);
    return buff.size();
}

short SkipStaticPointer(unsigned char *dictionary, int length, int index, FSM *fsm, short state, short *stateArray,
                        int position) {
    short *refer = stateArray + index - 1;
    short *cur = stateArray + DICTIONARY_BYTES + position;
    for (int pos = 0; pos < length; pos++, refer++, cur++) {
        if (state == *refer) {
            memcpy(cur, refer + 1, sizeof(short) * (length - pos));
#ifdef ACTION
            for (int i = 0; i < length - pos; i++) {
                if (fsm->accept[cur[i]] == true) g_match++;
            }
#endif
            return cur[length - pos - 1];
        } else {
            *cur = ScanByte(state, dictionary[index++], fsm);
#ifdef ACTION
            if (fsm->accept[state] == true) g_match++;
#endif
            g_scan++;
        }
    }
    return state;
}

short SkipDynamicPointer(unsigned char *contents, int length, int index, FSM *fsm, short state, short *stateArray,
                         int position) {
    short *cur = stateArray + DICTIONARY_BYTES + position;
    short *refer = cur + index - 1;
    unsigned char *token = contents + position + index; //refer str
    for (int pos = 0; pos < length; pos++, refer++, cur++, token++) {
        if (state == *refer) {
            if (cur - refer >= length) {
                memcpy(cur, refer + 1, sizeof(short) * (length - pos));
            } else {
                for (int i = 0; i < length - pos; i++) {
                    cur[i] = refer[i + 1];
                }
            }
#ifdef ACTION
            for (int i = 0; i < length - pos; i++) {
                if (fsm->accept[cur[i]] == true) g_match++;
            }
#endif
            return cur[length - pos - 1];
        } else {
            *cur = ScanByte(state, *token, fsm);
#ifdef ACTION
            if (fsm->accept[state] == true) g_match++;
#endif
            g_scan++;
        }
    }
    return state;
}

FSM *readFromFile(char *tableFile, char *acceptFile) {
    vector<int> acceptVec, vecTable;
    int temp_dat;

    ifstream in_ac(acceptFile);
    if (in_ac.is_open()) {
        while (in_ac >> temp_dat) {
            acceptVec.push_back(temp_dat);
        }
        in_ac.close();
    }

    ifstream in_table(tableFile);
    if (in_table.is_open()) {
        string temp_line;
        while (getline(in_table, temp_line)) {
            if (temp_line.size() > 2) {
                stringstream stream(temp_line);
                while (stream >> temp_dat) {
                    vecTable.push_back(temp_dat);
                }
            }
        }
        in_table.close();
    }

    int *list = new int[vecTable.size()];
    bool *accept = new bool[vecTable.size() / 256];

    for (size_t i = 0; i < vecTable.size(); i++) {
        list[i] = vecTable[i];
    }

    for (int state: acceptVec) {
        accept[state] = true;
    }

    FSM *fsm = new FSM;
    fsm->list = list;
    fsm->accept = accept;
    return fsm;
}


int readFiles(const vector<string> &names, vector<Content> &contents) {
    contents.resize(names.size());

    transform(names.begin(), names.end(), contents.begin(), [](const string &name) {
        std::ifstream file(name, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return Content{nullptr, 0};
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        auto *buffer = new unsigned char[static_cast<size_t>(size)];
        file.read(reinterpret_cast<char *>(buffer), size);
        return Content{buffer, static_cast<size_t>(size)};
    });
    return 0;
}
#include "utils.h"
#include <string>
#include "dictionary.h"
#include <algorithm>
#include <sys/time.h>
#include <iostream>

unsigned long g_scan = 0;
unsigned int g_match = 0;
int literal_num = 0;
clock_t spend = 0;
unsigned long long total = 73385030;
unsigned long long compress = 165299749;

enum SetType {
    Alexa,
    Google,
    Enwik,
    COVID_19
};

int main(int argc, char **argv) {
    if (argc != 5) {
        cout << "usage: " << argv[0] << " <input> <meta> <rule> <accept> <set>" << endl;
        return -1;
    }
    char *_input = argv[1];
    char *_meta = argv[2];
    char *_rule = argv[3];
    char *_accept = argv[4];
    int _set = atoi(argv[5]);

    switch (_set) {
        case SetType::Alexa: {
            total = 73385030;
            compress = 12397003;
            break;
        }
        case SetType::Google: {
            total = 226060405;
            compress = 53840635;
            break;
        }
        case SetType::Enwik: {
            total = 100000000;
            compress = 25742001;
            break;
        }
        case SetType::COVID_19: {
            total = 113096550;
            compress = 7364753;
            break;
        }
        default: {
            cout << "check your <set>:" << _set << endl;
            return -1;
        }
    }

    FSM *fsm = readFromFile(_rule, _accept);

    vector<string> inputNames, metaNames;

    if (!readFileName(_input, inputNames)) {
        cout << "check your <input>:" << _input << endl;
        return -1;
    }
    if (!readFileName(_meta, metaNames)) {
        cout << "check your <meta>:" << _meta << endl;
        return -1;
    }

    sort(inputNames.begin(), inputNames.end());
    sort(metaNames.begin(), metaNames.end());

    vector<Content> contents, metaInput;
    readFiles(inputNames,contents);
    readFiles(metaNames,metaInput);

    // todo
















    int size = contents.size();
    short *stateArray = new short[DICTIONARY_BYTES + 320000000];
    GetDictionaryState(stateArray, fsm);

    int *metaSize = new int[size];
    int *contentSize = new int[size];

    for (int i = 0; i < size; i++) {
        metaSize[i] = metaInput[i].size / sizeof(meta);
        contentSize[i] = contents[i].size;
    }

    short state = 0;
    int pointer_len = 0;
    int pointer_count = 0;
    int pos;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    for (int i = 0; i < size; i++) {
        unsigned char *content = contents[i].pBuff;
        meta *m = (meta *) metaInput[i].pBuff;
        int metaLength = metaSize[i];
        pos = 0;
        state = 0;
        int start = 0;
        for (int j = 0; j < metaLength; j += 3) {
            unsigned int uncompress = m[j].count;
            unsigned int copyLen = m[j + 1].copy_len;
            for (int k = 0; k < uncompress; k++) {
                ScanByte(state, content[pos], fsm);
                stateArray[pos + DICTIONARY_BYTES] = state;
                g_scan++;
                pos++;
#ifdef ACTION
                if (fsm->_accept[state] == true)
                {
                    g_match++;
                }
#endif
                literal_num++;
            }
            // scan pointer
            if (copyLen > 0) {
                int index = m[j + 2].index;
                if (index < 0) {
                    state = SkipDynamicPointer(content, copyLen, index, fsm, state, stateArray, pos);
                } else {
                    state = SkipStaticPointer(kBrotliDictionaryData, copyLen, index, fsm, state, stateArray, pos);
                }
                pos += copyLen;
                pointer_len += copyLen;
                pointer_count++;
            }
        }
    }
    gettimeofday(&tv, NULL);
    long end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    printf("time = %d\n", (end - start));
    printf("state = %d\n", state);
    printf("scan = %d\n", g_scan);
    printf("match = %d\n", g_match);
    //printf("scan ratio = %.2f\n", 100 * (g_scan * 1.0 / total));
    printf("skip ratio = %.2f\n", 100.0 - 100 * (g_scan * 1.0 / total));
    printf("pointer ratio = %.2f\n", 100 * (1.0 - (literal_num * 1.0 / total)));
    printf("compress ratio = %.2f\n", 100 * (compress * 1.0) / total);
    printf("average pointer len = %.2f\n", (pointer_len * 1.0) / pointer_count);

    delete[] fsm->accept;
    delete[] fsm->list;
    delete[] stateArray;
    delete fsm;
    for (int i = 0; i < contents.size(); i++) {
        delete contents[i].pBuff;
        delete metaInput[i].pBuff;
    }
}
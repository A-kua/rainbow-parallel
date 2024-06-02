#include <cstdlib>
#include <cstdio>
#include <malloc.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <memory>

using namespace std;

#define DICTIONARY_BYTES 122784


#define STRUCT(type) \
typedef struct _tag_##type type;\
struct _tag_##type

#define UNION(type) \
typedef union _tag_##type type;\
union _tag_##type

UNION(meta) {
    unsigned int count;
    unsigned int copy_len;
    int index;
};


STRUCT(Input) {
    unsigned char *contents;
    meta *meta;
    size_t metaLength;
};


STRUCT(Content) {
    unsigned char *pBuff;
    size_t size;
};


STRUCT(FSM) {
    int *list;
    bool *accept;
};

inline int ScanByte(short &state, unsigned char token, FSM *fsm) {
    state = fsm->list[state * 256 + token];
    return state;
};

int readFileName(char *path, vector<string> &name);

int LoadText(char *path, vector<Content> &buff, vector<string> &name);

void GetDictionaryState(short *DictionaryState, FSM *fsm);

int BrInit(vector<Content> &contentsBuf, vector<Content> &metaBuf, Input **Txt);

void Performance();

short naive_for_static(Input *token, int length, short *state_array, FSM *fsm, short state);

short naive_for_dynamic(Input *token, int length, short *state_array, FSM *fsm, short state);

short rainbow_for_dynamic(Input *token, int length, int dist, short *state_array, FSM *fsm, short state);

short
rainbow_for_static(Input *token, int length, int index, short *state_array, FSM *fsm, short state, short *dictionary);

FSM *readFromFile(char *tableFile, char *acceptFile);

short SkipStaticPointer(unsigned char *dictionary, int length, int index, FSM *fsm, short state, short *stateArray,
                        int position);

short SkipDynamicPointer(unsigned char *contents, int length, int index, FSM *fsm, short state, short *stateArray,
                         int position);

int readFiles(const vector<string> &names, vector<Content> &contents);
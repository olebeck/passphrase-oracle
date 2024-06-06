#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>

#include <psp2/kernel/clib.h> 
#define printf sceClibPrintf

#define PORT 12346
#define BUFFER_SIZE 1024

typedef struct SceVshSblSsCreatePassPhraseArgs { // size is 0x18
  SceUInt32 reserved; // ex: 0
  SceSize size; // Size of this structure
  char accountIdText[0x10]; // Taken from registry "/CONFIG/NP/account_id" and converted to ASCII
} SceVshSblSsCreatePassPhraseArgs;

int _vshSblSsCreatePassPhrase(SceVshSblSsCreatePassPhraseArgs *pArgs, void *pPassPhrase, SceSize *piPassPhraseSize);

int sceRegMgrGetKeyBin(const char *category, const char *name, void *buf, int size);


void string_to_hex(const char *input, char *output) {
    const char *hex_chars = "0123456789ABCDEF";
    int length = strlen(input);
    for (int i = 0; i < length; i++) {
        output[i * 2] = hex_chars[((unsigned char)input[i]) >> 4];
        output[i * 2 + 1] = hex_chars[((unsigned char)input[i]) & 0x0F];
    }
    //output[length * 2] = '\0'; // Null terminate the string
}

void vita_tcp_server() {
    SceNetSockaddrIn serverAddr;
    SceNetSockaddrIn clientAddr;
    int serverSocket;
    int clientSocket;
    char buffer[BUFFER_SIZE];
    int recvLength;

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    serverAddr.sin_family = SCE_NET_AF_INET;
    serverAddr.sin_addr.s_addr = SCE_NET_INADDR_ANY;
    serverAddr.sin_port = sceNetHtons(PORT);

    serverSocket = sceNetSocket("vita_tcp_server", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
    if (serverSocket < 0) {
        printf("Error creating socket: %d\n", serverSocket);
        return;
    }

    if (sceNetBind(serverSocket, (SceNetSockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error binding socket\n");
        return;
    }

    if (sceNetListen(serverSocket, 128) < 0) {
        printf("Error listening on socket\n");
        return;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        int clientAddrSize = sizeof(clientAddr);
        clientSocket = sceNetAccept(serverSocket, (SceNetSockaddr *)&clientAddr, &clientAddrSize);
        if (clientSocket < 0) {
            printf("Error accepting connection: %d\n", clientSocket);
            continue;
        }

        printf("Connection accepted from %s\n", inet_ntoa(clientAddr.sin_addr));


        SceVshSblSsCreatePassPhraseArgs args = {
            .reserved = 0,
            .size = sizeof(SceVshSblSsCreatePassPhraseArgs),
        };

        char accountId[8];
        sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &accountId, sizeof(accountId));
        string_to_hex(accountId, args.accountIdText);

        char passphrase[0x200] = {0};
        uint64_t passphraseSize = sizeof(passphrase);
        int err = _vshSblSsCreatePassPhrase(&args, &passphrase, &passphraseSize);
        printf("err: %08x\n", err);
        if(err == 0) {
            sceNetSend(clientSocket, passphrase, passphraseSize, 0);
        } else {
            const char *message2 = "Error.\n";
            sceNetSend(clientSocket, message2, strlen(message2), 0);
        }

        // Close connection
        sceNetSocketClose(clientSocket);
    }

    sceNetSocketClose(serverSocket);
}

int main(int argc, char *argv[]) {
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam netInitParam;
	int size = 1*1024*1024;
	netInitParam.memory = malloc(size);
	netInitParam.size = size;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);

    vita_tcp_server();
    return 0;
}
#define WINVER 0x0502
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sddl.h>
#include <string>
#include <cstring>


using namespace std;


static PSECURITY_DESCRIPTOR create_security_descriptor()
{
    const char* sddl = "D:(A;OICI;GRGW;;;AU)(A;OICI;GA;;;BA)";
    PSECURITY_DESCRIPTOR security_descriptor = NULL;
    ConvertStringSecurityDescriptorToSecurityDescriptor(sddl, SDDL_REVISION_1, &security_descriptor, NULL);
    return security_descriptor;
}

static SECURITY_ATTRIBUTES create_security_attributes()
{
    SECURITY_ATTRIBUTES attributes;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = create_security_descriptor();
    attributes.bInheritHandle = FALSE;
    return attributes;
}


BOOL WINAPI MakeSlot(LPCTSTR lpszSlotName, HANDLE& hSlot, bool& serverStatus)
{
    auto attributes = create_security_attributes();
    hSlot = CreateMailslot(lpszSlotName, 0, MAILSLOT_WAIT_FOREVER, &attributes);

    if (hSlot == INVALID_HANDLE_VALUE)
    {
        auto error = GetLastError();

        if (error == ERROR_INVALID_NAME || error == ERROR_ALREADY_EXISTS)
        {
            serverStatus = FALSE;
            hSlot = CreateFile(lpszSlotName, GENERIC_WRITE, FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

            if (hSlot == INVALID_HANDLE_VALUE)
            {
                auto error = GetLastError();
                printf("Connection error! Error code: %d\n", GetLastError());
                return FALSE;
            }
        }
        else
        {
            printf("Mailslot creation error! Error code: %d\n", GetLastError());
            return FALSE;
        }
    }
    else
        serverStatus = TRUE;

    return TRUE;
}


BOOL Check(HANDLE hSlot, DWORD* cbMessage, DWORD* cMessage)
{
    DWORD cbMessageL, cMessageL;
    BOOL fResult;

    fResult = GetMailslotInfo(hSlot, (LPDWORD)NULL, &cbMessageL, &cMessageL, (LPDWORD)NULL);

    if (fResult)
    {
        printf("Found %i messages!\n", cMessageL);
        if (cbMessage != NULL)
            *cbMessage = cbMessageL;
        if (cMessage != NULL)
            *cMessage = cMessageL;
        return TRUE;
    }
    else
    {
        printf("GetMailslotInfo() failed! Error code: %d\n", GetLastError());
        return FALSE;
    }
}


BOOL Write(HANDLE hSlot, LPCTSTR lpszMessage)
{
    BOOL fResult;
    DWORD cbWritten;

    fResult = WriteFile(hSlot, lpszMessage, (lstrlen(lpszMessage) + 1) * sizeof(TCHAR), &cbWritten, (LPOVERLAPPED)NULL);

    if (fResult != 0)
    {
        printf("Message sent successfully!\n");
        return TRUE;
    }
    else
    {
        printf("WriteFile() failed! Error code: %d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}


BOOL Read(HANDLE hSlot)
{
    DWORD cbMessage, cMessage, cbRead;
    BOOL fResult;

    cbMessage = cMessage = cbRead = 0;

    fResult = GetMailslotInfo(hSlot, NULL, &cbMessage, &cMessage, NULL);

    if (cbMessage == MAILSLOT_NO_MESSAGE)
    {
        printf("No messages found in the mailslot!\n");
        return TRUE;
    }

    if (fResult == 0)
    {
        printf("GetMailslotInfo() failed! Error code: %d\n", GetLastError());
        return FALSE;
    }

    string Message(cbMessage, '\0');
    DWORD nNumberOfBytesToRead = sizeof(Message);

    fResult = ReadFile(hSlot, &Message[0], nNumberOfBytesToRead, &cbRead, NULL);

    if (fResult == 0)
    {
        printf("ReadFile() failed! Error code: %d\n", GetLastError());
        return FALSE;
    }
    cout << Message;
    return TRUE;
}


int main()
{
    DWORD* cbMessage;
    DWORD* cMessage;
    cbMessage = cMessage = 0;
    string MsName;

    cout << "Enter full mailslot path:";
    cin >> MsName;

    LPCTSTR SLOT_NAME = MsName.c_str();

    HANDLE hSlot;
    bool serverStatus;

    bool result = MakeSlot(SLOT_NAME, hSlot, serverStatus);
    if (result == FALSE)
        return 0;

    printf("-------Start session in ");
    printf((serverStatus) ? "Server-------\n" : "Client-------\n");
    printf((serverStatus) ? "Commands:\n   <read> to read the next one\n" : "   <write> to write a message\n");    
    printf("   <check> to check for messages\n");
    printf("   <quit> to terminate\n\n");

    string command;

    while (TRUE)
    {
        cout << ">";
        cin >> command;
        if (command == "check")
        {
            Check(hSlot, cbMessage, cMessage);
        }
        else if (command == "quit")
        {
            CloseHandle(hSlot);
            return 0;
        }
        else if ((command == "read") && (serverStatus))
        {
            Read(hSlot);
        }
        else if ((command == "write") && (!serverStatus))
        {
            printf("Enter message text. Terminate with an empty line.\n");

            string message, str;

            getline(cin, str);
            do
            {
                getline(cin, str);
                message += str + '\n';
            } while (!str.empty());

            Write(hSlot, (LPCTSTR)message.c_str());
        }
        else if ((command != "write") || (command != "check") || (command != "read") || (command == "quit"))
        {
            printf("Invalid command. Enter:\n");
            printf((serverStatus) ? "   <read> to read the next one\n" : "   <write> to write a message\n");
            printf("   <check> to check for messages\n");
            printf("   <quit> to terminate\n\n");
        }
    }
    return 0;
}

#include <Windows.h>
#include <TlHelp32.h>
#include<stdio.h>
#include "resource.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

//ȫ�ֱ���
HINSTANCE g_hInstance;
HWND g_hwndDlg;

// ��Ȩ����������ΪDEBUGȨ��
BOOL EnableDebugPrivilege();

//����ģ��
void EnumModules(HWND hListProcess);

//���Ի�����Ϣ������
BOOL CALLBACK DialogProc(
	HWND hwndDlg,  // handle to dialog box
	UINT uMsg,     // message
	WPARAM wParam, // first message parameter
	LPARAM lParam  // second message parameter
);
//����ProcessListView���
void InitProcessListView(HWND hwndDlg);
//����ModulesListView���
void InitModulesListView(HWND hwndDlg);
// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
)
{
	EnableDebugPrivilege();
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	g_hInstance = hInstance;

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, DialogProc);

}

// ��Ȩ����������ΪDEBUGȨ��
BOOL EnableDebugPrivilege()
{
	HANDLE hToken;
	BOOL fOk = FALSE;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);

		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return fOk;
}

void EnumModules(HWND hListProcess)
{
	DWORD dwRowId;
	TCHAR szPid[0x20];
	LV_ITEM lv;

	//��ʼ��
	memset(&lv, 0, sizeof(LV_ITEM));
	memset(szPid, 0, 0x20);
	//��ȡѡ����
	dwRowId = SendMessage(hListProcess, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
	if (dwRowId == -1)
	{
		MessageBox(NULL, L"��ѡ�����", L"������", MB_OK);
		return;
	}
	//��ȡPID
	lv.iSubItem = 1;
	lv.pszText = szPid;
	lv.cchTextMax = 0x20;
	SendMessage(hListProcess, LVM_GETITEMTEXT, dwRowId, (DWORD)&lv);

	//���б��������Ϣ
	hListProcess = GetDlgItem(g_hwndDlg, IDC_LIST_MODULES);
	//MessageBox(NULL, szPid, L"PID", MB_OK);

	//��PIDȥ��ȡ���̵ĵ���ģ��
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _wtoi(szPid));
	if (INVALID_HANDLE_VALUE == hSnapshot)
	{
		MessageBox(NULL, L"û��Ȩ�޻�ȡ����ģ��", L"������", MB_OK);
		return;
	}
	MODULEENTRY32 mi;
	mi.dwSize = sizeof(MODULEENTRY32); // ��һ��ʹ�ñ����ʼ����Ա
	BOOL  bRet = Module32First(hSnapshot, &mi);

	TCHAR szlpModulePathBuffer[MAX_PATH] = { 0 };
	TCHAR szlpModuleBaseBuffer[0x10] = { 0 };
	TCHAR szlpModuleSizeBuffer[0x10] = { 0 };

	LV_ITEM vitem;
	//��ʼ��
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;
	size_t n = 0;
	while (bRet)
	{
		wsprintf(szlpModulePathBuffer, TEXT("%ws"), mi.szExePath);
		wsprintf(szlpModuleBaseBuffer, TEXT("%08X"), mi.modBaseAddr);
		wsprintf(szlpModuleSizeBuffer, TEXT("%08X"), mi.modBaseSize);

		vitem.pszText = szlpModulePathBuffer;
		vitem.iItem = n;
		vitem.iSubItem = 0;
		//ListView_InsertItem(hListProcess, &vitem);								
		SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);

		vitem.pszText = szlpModuleBaseBuffer;
		vitem.iItem = n;
		vitem.iSubItem = 1;
		ListView_SetItem(hListProcess, &vitem); 
		
		vitem.pszText = szlpModuleSizeBuffer;
		vitem.iItem = n;
		vitem.iSubItem = 2;
		ListView_SetItem(hListProcess, &vitem);

		bRet = Module32Next(hSnapshot, &mi);
		n++;
	}

}


BOOL CALLBACK DialogProc(
	HWND hwndDlg,  // handle to dialog box			
	UINT uMsg,     // message			
	WPARAM wParam, // first message parameter			
	LPARAM lParam  // second message parameter			
)
{
	g_hwndDlg = hwndDlg;
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//����ͼ��
		HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
		//����ͼ��
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (long)hIcon);
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (long)hIcon);
		//����ProcessListView���
		InitProcessListView(hwndDlg);
		//����ModulesListView���
		InitModulesListView(hwndDlg);
		return TRUE;
	}
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return TRUE;

	case WM_NOTIFY: 
	{
		NMHDR* pNMHDR = (NMHDR*)lParam;
		if (wParam == IDC_LIST_PROCESS && pNMHDR->code == NM_CLICK)
		{
			EnumModules(GetDlgItem(hwndDlg, IDC_LIST_PROCESS));
		}
		return FALSE;
	}

	case  WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case   IDC_BUTTON_ABOUT:
			DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hwndDlg, About);
			return TRUE;

		case   IDC_BUTTON_EXIT:
			EndDialog(hwndDlg, 0);

			return TRUE;
		}
		break;
	}

	return FALSE;
}
void InitProcessListView(HWND hDlg)
{
	//��ʼ������
	LV_COLUMN lv;
	HWND hListProcess;

	//��ʼ��								
	memset(&lv, 0, sizeof(LV_COLUMN));
	//��ȡIDC_LIST_PROCESS���								
	hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//��������ѡ��								
	SendMessage(hListProcess, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//��һ��								
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = (LPTSTR)(TEXT("����"));			//�б���				
	lv.cx = 200;								//�п�
	lv.iSubItem = 0;
	//ListView_InsertColumn(hListProcess, 0, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
	//�ڶ���								
	lv.pszText = (LPTSTR)(TEXT("PID"));
	lv.cx = 100;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
	//������								
	lv.pszText = (LPTSTR)(TEXT("�����ַ"));
	lv.cx = 100;
	lv.iSubItem = 2;
	ListView_InsertColumn(hListProcess, 2, &lv);
	//������								
	lv.pszText = (LPTSTR)(TEXT("�����С"));
	lv.cx = 100;
	lv.iSubItem = 3;
	ListView_InsertColumn(hListProcess, 3, &lv);

	//���б��������Ϣ
	LV_ITEM vitem;

	//��ʼ��
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	// ��ȡ���̵�PID
	HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//��ȡ���̿��վ��
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	BOOL flag = Process32First(hProcSnap, &pe32);//��ȡ�б�ĵ�һ������
	size_t n = 0;
	TCHAR szlpProcessNameBuffer[0x50] = { 0 };
	TCHAR szlpPidBuffer[0x10] = { 0 };
	TCHAR szlpImagebaseBuffer[0x10] = { 0 };
	TCHAR szlpImageSizeBuffer[0x10] = { 0 };
	
	while (flag)
	{
		wsprintf(szlpProcessNameBuffer, TEXT("%ws"), pe32.szExeFile);
		wsprintf(szlpPidBuffer, TEXT("%d"), pe32.th32ProcessID);
		wsprintf(szlpImagebaseBuffer, TEXT("%08X"), GetModuleHandle(pe32.szExeFile));
		wsprintf(szlpImageSizeBuffer, TEXT("%08d"), pe32.dwSize);


		vitem.pszText = szlpProcessNameBuffer;
		vitem.iItem = n;
		vitem.iSubItem = 0;
		//ListView_InsertItem(hListProcess, &vitem);								
		SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);

		vitem.pszText = szlpPidBuffer;
		vitem.iItem = n;
		vitem.iSubItem = 1;
		ListView_SetItem(hListProcess, &vitem);

		vitem.pszText = szlpImagebaseBuffer;
		vitem.iItem = n;
		vitem.iSubItem = 2;
		ListView_SetItem(hListProcess, &vitem);

		vitem.pszText = szlpImageSizeBuffer;
		vitem.iItem = n;
		vitem.iSubItem = 3;
		ListView_SetItem(hListProcess, &vitem);

		flag = Process32Next(hProcSnap, &pe32);//��ȡ��һ������
		n++;
	}
}
void InitModulesListView(HWND hDlg)
{
	LV_COLUMN lv;
	HWND hListModules;

	//��ʼ��								
	memset(&lv, 0, sizeof(LV_COLUMN));
	//��ȡIDC_LIST_PROCESS���								
	hListModules = GetDlgItem(hDlg, IDC_LIST_MODULES);
	//��������ѡ��								
	SendMessage(hListModules, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//��һ��								
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = (LPTSTR)(TEXT("ģ��λ��"));			//�б���				
	lv.cx = 250;								//�п�
	lv.iSubItem = 0;
	//ListView_InsertColumn(hListProcess, 0, &lv);								
	SendMessage(hListModules, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
	//�ڶ���								
	lv.pszText = (LPTSTR)(TEXT("ģ���ַ"));
	lv.cx = 125;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListModules, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
	//������								
	lv.pszText = (LPTSTR)(TEXT("ģ���С"));
	lv.cx = 125;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListModules, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
}
// �����ڡ������Ϣ�������
BOOL CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case   IDC_BUTTON_OK:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}
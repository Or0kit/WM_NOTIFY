#include <Windows.h>
#include <TlHelp32.h>
#include<stdio.h>
#include "resource.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

//全局变量
HINSTANCE g_hInstance;
HWND g_hwndDlg;

// 提权函数：提升为DEBUG权限
BOOL EnableDebugPrivilege();

//遍历模块
void EnumModules(HWND hListProcess);

//主对话框消息处理函数
BOOL CALLBACK DialogProc(
	HWND hwndDlg,  // handle to dialog box
	UINT uMsg,     // message
	WPARAM wParam, // first message parameter
	LPARAM lParam  // second message parameter
);
//设置ProcessListView风格
void InitProcessListView(HWND hwndDlg);
//设置ModulesListView风格
void InitModulesListView(HWND hwndDlg);
// “关于”框的消息处理程序。
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

// 提权函数：提升为DEBUG权限
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

	//初始化
	memset(&lv, 0, sizeof(LV_ITEM));
	memset(szPid, 0, 0x20);
	//获取选择行
	dwRowId = SendMessage(hListProcess, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
	if (dwRowId == -1)
	{
		MessageBox(NULL, L"请选择进程", L"出错啦", MB_OK);
		return;
	}
	//获取PID
	lv.iSubItem = 1;
	lv.pszText = szPid;
	lv.cchTextMax = 0x20;
	SendMessage(hListProcess, LVM_GETITEMTEXT, dwRowId, (DWORD)&lv);

	//向列表中添加信息
	hListProcess = GetDlgItem(g_hwndDlg, IDC_LIST_MODULES);
	//MessageBox(NULL, szPid, L"PID", MB_OK);

	//以PID去获取进程的导入模块
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _wtoi(szPid));
	if (INVALID_HANDLE_VALUE == hSnapshot)
	{
		MessageBox(NULL, L"没有权限获取进程模块", L"出错啦", MB_OK);
		return;
	}
	MODULEENTRY32 mi;
	mi.dwSize = sizeof(MODULEENTRY32); // 第一次使用必须初始化成员
	BOOL  bRet = Module32First(hSnapshot, &mi);

	TCHAR szlpModulePathBuffer[MAX_PATH] = { 0 };
	TCHAR szlpModuleBaseBuffer[0x10] = { 0 };
	TCHAR szlpModuleSizeBuffer[0x10] = { 0 };

	LV_ITEM vitem;
	//初始化
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
		//加载图标
		HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
		//设置图标
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (long)hIcon);
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (long)hIcon);
		//设置ProcessListView风格
		InitProcessListView(hwndDlg);
		//设置ModulesListView风格
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
	//初始化列名
	LV_COLUMN lv;
	HWND hListProcess;

	//初始化								
	memset(&lv, 0, sizeof(LV_COLUMN));
	//获取IDC_LIST_PROCESS句柄								
	hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//设置整行选中								
	SendMessage(hListProcess, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//第一列								
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = (LPTSTR)(TEXT("进程"));			//列标题				
	lv.cx = 200;								//列宽
	lv.iSubItem = 0;
	//ListView_InsertColumn(hListProcess, 0, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
	//第二列								
	lv.pszText = (LPTSTR)(TEXT("PID"));
	lv.cx = 100;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
	//第三列								
	lv.pszText = (LPTSTR)(TEXT("镜像基址"));
	lv.cx = 100;
	lv.iSubItem = 2;
	ListView_InsertColumn(hListProcess, 2, &lv);
	//第四列								
	lv.pszText = (LPTSTR)(TEXT("镜像大小"));
	lv.cx = 100;
	lv.iSubItem = 3;
	ListView_InsertColumn(hListProcess, 3, &lv);

	//向列表中填充信息
	LV_ITEM vitem;

	//初始化
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	// 获取进程的PID
	HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//获取进程快照句柄
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	BOOL flag = Process32First(hProcSnap, &pe32);//获取列表的第一个进程
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

		flag = Process32Next(hProcSnap, &pe32);//获取下一个进程
		n++;
	}
}
void InitModulesListView(HWND hDlg)
{
	LV_COLUMN lv;
	HWND hListModules;

	//初始化								
	memset(&lv, 0, sizeof(LV_COLUMN));
	//获取IDC_LIST_PROCESS句柄								
	hListModules = GetDlgItem(hDlg, IDC_LIST_MODULES);
	//设置整行选中								
	SendMessage(hListModules, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//第一列								
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = (LPTSTR)(TEXT("模块位置"));			//列标题				
	lv.cx = 250;								//列宽
	lv.iSubItem = 0;
	//ListView_InsertColumn(hListProcess, 0, &lv);								
	SendMessage(hListModules, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
	//第二列								
	lv.pszText = (LPTSTR)(TEXT("模块基址"));
	lv.cx = 125;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListModules, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
	//第三列								
	lv.pszText = (LPTSTR)(TEXT("模块大小"));
	lv.cx = 125;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListModules, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
}
// “关于”框的消息处理程序。
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
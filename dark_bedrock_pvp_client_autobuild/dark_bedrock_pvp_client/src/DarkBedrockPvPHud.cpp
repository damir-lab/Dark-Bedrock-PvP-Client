#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <chrono>

namespace {
HINSTANCE g_instance{};
HWND g_overlay{};
HANDLE g_stop{};
bool g_visible = true;
bool g_panel = true;
bool g_cps = true, g_keys = true, g_fps = true, g_crosshair = true, g_ping = true;
int g_fpsEstimate = 0, g_frames = 0;
auto g_fpsStart = std::chrono::steady_clock::now();
std::vector<std::chrono::steady_clock::time_point> g_clicks[2];
bool g_prevMouse[2]{};
bool g_prevInsert{}, g_prevF6{}, g_prevF7{}, g_prevF8{}, g_prevF9{}, g_prevF10{}, g_prevF11{};

COLORREF C_BG = RGB(16,18,24), C_PANEL = RGB(26,29,38), C_ACCENT = RGB(116,145,255);
COLORREF C_TEXT = RGB(238,240,247), C_MUTED = RGB(151,159,177), C_GREEN = RGB(104,220,164);

bool PressedEdge(int vk, bool& previous) {
    bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool edge = now && !previous; previous = now; return edge;
}
void FillRound(HDC dc,int x,int y,int w,int h,int rad,COLORREF color) {
    HBRUSH b=CreateSolidBrush(color); HPEN p=CreatePen(PS_NULL,0,color);
    auto ob=SelectObject(dc,b); auto op=SelectObject(dc,p);
    RoundRect(dc,x,y,x+w,y+h,rad,rad);
    SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(b); DeleteObject(p);
}
void DrawText(HDC dc,int x,int y,const std::wstring& s,COLORREF c,int size=16,int weight=FW_NORMAL) {
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,c);
    HFONT f=CreateFontW(size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
    auto old=SelectObject(dc,f); TextOutW(dc,x,y,s.c_str(),(int)s.size());
    SelectObject(dc,old); DeleteObject(f);
}
int CPS(int b) {
    auto now=std::chrono::steady_clock::now();
    while(!g_clicks[b].empty() && now-g_clicks[b].front()>std::chrono::seconds(1)) g_clicks[b].erase(g_clicks[b].begin());
    return (int)g_clicks[b].size();
}
void ClickPoll() {
    auto now=std::chrono::steady_clock::now();
    for(int i=0;i<2;i++) {
        bool down=(GetAsyncKeyState(i==0?VK_LBUTTON:VK_RBUTTON)&0x8000)!=0;
        if(down&&!g_prevMouse[i]) g_clicks[i].push_back(now);
        g_prevMouse[i]=down;
    }
    while(!g_clicks[0].empty()&&now-g_clicks[0].front()>std::chrono::seconds(1))g_clicks[0].erase(g_clicks[0].begin());
    while(!g_clicks[1].empty()&&now-g_clicks[1].front()>std::chrono::seconds(1))g_clicks[1].erase(g_clicks[1].begin());
}
void PollHotkeys() {
    if(PressedEdge(VK_INSERT,g_prevInsert)) { g_visible=!g_visible; ShowWindow(g_overlay,g_visible?SW_SHOW:SW_HIDE); }
    if(PressedEdge(VK_F6,g_prevF6)) g_panel=!g_panel;
    if(PressedEdge(VK_F7,g_prevF7)) g_cps=!g_cps;
    if(PressedEdge(VK_F8,g_prevF8)) g_keys=!g_keys;
    if(PressedEdge(VK_F9,g_prevF9)) g_fps=!g_fps;
    if(PressedEdge(VK_F10,g_prevF10)) g_crosshair=!g_crosshair;
    if(PressedEdge(VK_F11,g_prevF11)) g_ping=!g_ping;
}
void DrawHud(HDC dc, int width, int height) {
    // Color-key background stays transparent.
    int y=26;
    FillRound(dc,22,y,245,52,12,C_PANEL);
    FillRound(dc,34,y+12,28,28,7,C_ACCENT);
    DrawText(dc,43,y+15,L"D",C_TEXT,18,FW_BOLD);
    DrawText(dc,73,y+9,L"DARK  /  BEDROCK",C_TEXT,15,FW_BOLD);
    DrawText(dc,73,y+30,L"VISUAL PVP HUD",C_MUTED,10,FW_NORMAL);
    y+=64;
    if(g_cps) {
        FillRound(dc,22,y,150,65,10,C_PANEL);
        DrawText(dc,34,y+8,L"CLICKS PER SECOND",C_MUTED,10,FW_BOLD);
        DrawText(dc,34,y+28,L"LMB",C_TEXT,12,FW_BOLD);
        DrawText(dc,83,y+24,std::to_wstring(CPS(0)),C_ACCENT,20,FW_BOLD);
        DrawText(dc,34,y+47,L"RMB",C_TEXT,12,FW_BOLD);
        DrawText(dc,83,y+43,std::to_wstring(CPS(1)),C_GREEN,16,FW_BOLD);
        y+=75;
    }
    if(g_fps) {
        FillRound(dc,22,y,150,48,10,C_PANEL);
        DrawText(dc,34,y+7,L"OVERLAY FPS*",C_MUTED,10,FW_BOLD);
        DrawText(dc,34,y+22,std::to_wstring(g_fpsEstimate),C_TEXT,18,FW_BOLD); y+=58;
    }
    if(g_ping) {
        FillRound(dc,22,y,150,48,10,C_PANEL);
        DrawText(dc,34,y+7,L"SERVER PING",C_MUTED,10,FW_BOLD);
        DrawText(dc,34,y+22,L"N/A",C_MUTED,18,FW_BOLD);
    }
    if(g_keys) {
        int x=width-145, top=28, k=34, gap=5;
        const wchar_t* labels[]={L"W",L"A",L"S",L"D"};
        int vks[]={'W','A','S','D'};
        int xs[]={x+k+gap,x,x+k+gap,x+2*(k+gap)};
        int ys[]={top,top+k+gap,top+k+gap,top+k+gap};
        for(int i=0;i<4;i++) {
            bool down=(GetAsyncKeyState(vks[i])&0x8000)!=0;
            FillRound(dc,xs[i],ys[i],k,k,7,down?C_ACCENT:C_PANEL);
            DrawText(dc,xs[i]+11,ys[i]+8,labels[i],C_TEXT,13,FW_BOLD);
        }
        FillRound(dc,x,top+2*(k+gap),3*k+2*gap,25,7,(GetAsyncKeyState(VK_SPACE)&0x8000)?C_ACCENT:C_PANEL);
        DrawText(dc,x+38,top+2*(k+gap)+5,L"SPACE",C_TEXT,11,FW_BOLD);
    }
    if(g_crosshair) {
        int cx=width/2,cy=height/2;
        HPEN pen=CreatePen(PS_SOLID,2,C_ACCENT); auto old=SelectObject(dc,pen);
        MoveToEx(dc,cx-9,cy,nullptr);LineTo(dc,cx-3,cy);
        MoveToEx(dc,cx+3,cy,nullptr);LineTo(dc,cx+9,cy);
        MoveToEx(dc,cx,cy-9,nullptr);LineTo(dc,cx,cy-3);
        MoveToEx(dc,cx,cy+3,nullptr);LineTo(dc,cx,cy+9);
        SelectObject(dc,old);DeleteObject(pen);
    }
    if(g_panel) {
        int px=width-254,py=140;
        FillRound(dc,px,py,232,238,12,C_PANEL);
        DrawText(dc,px+14,py+12,L"CLICKGUI  /  MODULES",C_TEXT,13,FW_BOLD);
        DrawText(dc,px+14,py+36,L"F7   CPS",g_cps?C_GREEN:C_MUTED,12,FW_BOLD);
        DrawText(dc,px+14,py+66,L"F8   KEYSTROKES",g_keys?C_GREEN:C_MUTED,12,FW_BOLD);
        DrawText(dc,px+14,py+96,L"F9   FPS ESTIMATE",g_fps?C_GREEN:C_MUTED,12,FW_BOLD);
        DrawText(dc,px+14,py+126,L"F10  CROSSHAIR",g_crosshair?C_GREEN:C_MUTED,12,FW_BOLD);
        DrawText(dc,px+14,py+156,L"F11  PING LABEL",g_ping?C_GREEN:C_MUTED,12,FW_BOLD);
        DrawText(dc,px+14,py+194,L"INSERT  hide/show HUD",C_MUTED,10);
        DrawText(dc,px+14,py+210,L"F6  toggle this panel",C_MUTED,10);
    }
    DrawText(dc,22,height-27,L"*Overlay refresh estimate, not game FPS. Ping unavailable.",C_MUTED,10);
}
LRESULT CALLBACK OverlayProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_TIMER: {
        PollHotkeys(); ClickPoll();
        auto now=std::chrono::steady_clock::now(); ++g_frames;
        if(now-g_fpsStart>=std::chrono::seconds(1)){g_fpsEstimate=g_frames;g_frames=0;g_fpsStart=now;}
        InvalidateRect(hwnd,nullptr,FALSE); return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps); RECT r; GetClientRect(hwnd,&r);
        HBRUSH bg=CreateSolidBrush(RGB(1,1,1)); FillRect(dc,&r,bg); DeleteObject(bg);
        DrawHud(dc,r.right,r.bottom); EndPaint(hwnd,&ps); return 0;
    }
    case WM_DESTROY: KillTimer(hwnd,1); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
DWORD WINAPI OverlayThread(void*) {
    WNDCLASSW wc{}; wc.lpfnWndProc=OverlayProc; wc.hInstance=g_instance;
    wc.lpszClassName=L"DarkBedrockPvPHudOverlay"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);
    int w=GetSystemMetrics(SM_CXSCREEN),h=GetSystemMetrics(SM_CYSCREEN);
    g_overlay=CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW,
        wc.lpszClassName,L"Dark Bedrock PvP HUD",WS_POPUP,0,0,w,h,nullptr,nullptr,g_instance,nullptr);
    if(!g_overlay) return 0;
    SetLayeredWindowAttributes(g_overlay,RGB(1,1,1),255,LWA_COLORKEY);
    SetTimer(g_overlay,1,16,nullptr);
    ShowWindow(g_overlay,SW_SHOW); UpdateWindow(g_overlay);
    MSG msg; while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return 0;
}
}

extern "C" __declspec(dllexport) const wchar_t* __cdecl DarkBedrockPvPHud_Version() {
    return L"0.2.0-visual-only";
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        g_instance=instance; DisableThreadLibraryCalls(instance);
        HANDLE t=CreateThread(nullptr,0,OverlayThread,nullptr,0,nullptr);
        if(t) CloseHandle(t);
    }
    return TRUE;
}

// Componente novo: licença própria em LICENCA-PAPEL-VIVO.txt; exceções LGPL
// preservadas. Painel Win32: arte incorporada, animação breve sob demanda e
// diagnóstico local.
Gdiplus::Bitmap *banner = nullptr;
IStream *bannerStream = nullptr;
std::wstring logPath, logText;
HWND logWindow = nullptr, logEdit = nullptr;
void setupLog() {
  auto dir = std::filesystem::path(iniPath).parent_path() / L"Logs";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  logPath = (dir / L"PapelVivo.log").wstring();
  if (std::filesystem::file_size(logPath, ec) > 1024 * 1024 && !ec) {
    std::filesystem::remove(dir / L"PapelVivo-anterior.log", ec);
    std::filesystem::rename(logPath, dir / L"PapelVivo-anterior.log", ec);
  }
  std::ifstream in{std::filesystem::path(logPath)};
  logText = fromUtf8(std::string(std::istreambuf_iterator<char>(in), {}));
  if (logText.size() > 180000)
    logText = logText.substr(logText.size() - 180000);
}
void recordLog(const std::wstring &level, const std::wstring &text) {
  SYSTEMTIME t;
  GetLocalTime(&t);
  wchar_t date[80];
  swprintf(date, 80, L"%04u-%02u-%02u %02u:%02u:%02u.%03u", t.wYear, t.wMonth,
           t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
  std::wstring line = std::wstring(date) + L" [" + pvlang::text(level) + L"] " +
                      pvlang::text(text) + L"\r\n";
  logText += line;
  if (logText.size() > 180000)
    logText.erase(0, logText.size() - 180000);
  std::error_code ec;
  auto size = std::filesystem::file_size(logPath, ec);
  if (!ec && size > 2 * 1024 * 1024) {
    auto old = std::filesystem::path(logPath).parent_path() /
               L"PapelVivo-anterior.log";
    std::filesystem::remove(old, ec);
    std::filesystem::rename(logPath, old, ec);
  }
  std::ofstream f(std::filesystem::path(logPath),
                  std::ios::app | std::ios::binary);
  f << toUtf8(line);
  if (logEdit) {
    SetWindowTextW(logEdit, logText.c_str());
    SendMessageW(logEdit, EM_SETSEL, logText.size(), logText.size());
    SendMessageW(logEdit, EM_SCROLLCARET, 0, 0);
  }
}
void drawButton(DRAWITEMSTRUCT*);
LRESULT CALLBACK ButtonMotion(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
UiSurface logSurface;
LRESULT CALLBACK LogProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (m == WM_CREATE) {
    logEdit =
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", logText.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                        16, 136, 768, 332, h, (HMENU)600, instance, nullptr);
    SendMessageW(logEdit, EM_SETLIMITTEXT, 250000, 0);
    SendMessageW(logEdit, WM_SETFONT, (WPARAM)normalFont, TRUE);
    HWND label = CreateWindowW(L"STATIC",
                               (pvlang::english?L"Device logs. Review personal paths before sharing.":L"Registros do aparelho. Revise caminhos pessoais antes de compartilhar."),
                               WS_CHILD | WS_VISIBLE, 16, 96, 768, 32, h,
                               nullptr, instance, nullptr);
    SendMessageW(label, WM_SETFONT, (WPARAM)normalFont, TRUE);
    const wchar_t *names[] = {pvlang::english?L"Copy report":L"Copiar relatório",pvlang::english?L"Open logs folder":L"Abrir pasta de logs",pvlang::english?L"Close":L"Fechar"};
    for (int i = 0; i < 3; i++) {
      HWND b = CreateWindowW(
          L"BUTTON", names[i], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 16 + i * 256,
          484, 240, 44, h, (HMENU)(INT_PTR)(601 + i), instance, nullptr);
      SendMessageW(b, WM_SETFONT, (WPARAM)normalFont, TRUE);SetWindowSubclass(b,ButtonMotion,1,0);
    }
    return 0;
  }
  if (m == WM_COMMAND) {
    if (LOWORD(w) == 601) {
      const std::wstring report = L"Papel Vivo 2.0.1 | autor kz\r\n" +
                                  renderInfo + L"\r\n\r\n" + logText;
      HGLOBAL data =
          GlobalAlloc(GMEM_MOVEABLE, (report.size() + 1) * sizeof(wchar_t));
      if (data) {
        void *p = GlobalLock(data);
        if (p) {
          memcpy(p, report.c_str(), (report.size() + 1) * sizeof(wchar_t));
          GlobalUnlock(data);
          if (OpenClipboard(h)) {
            EmptyClipboard();
            if (SetClipboardData(CF_UNICODETEXT, data))
              data = nullptr;
            CloseClipboard();
          }
        }
        if (data)
          GlobalFree(data);
      }
    } else if (LOWORD(w) == 602) {
      auto dir = std::filesystem::path(logPath).parent_path().wstring();
      wchar_t root[32768]{};if(GetEnvironmentVariableW(L"LOCALAPPDATA",root,32768))dir=(std::filesystem::path(root)/L"PapelVivo"/L"Player"/L"Logs").wstring();std::filesystem::create_directories(dir);
      ShellExecuteW(h, L"open", dir.c_str(), nullptr, nullptr, SW_SHOW);
    } else if (LOWORD(w) == 603)
      DestroyWindow(h);
    return 0;
  }
  if(m==WM_CTLCOLORSTATIC||m==WM_CTLCOLOREDIT){SetBkColor((HDC)w,RGB(21,17,34));SetTextColor((HDC)w,RGB(250,230,242));return (LRESULT)bg;}
  if(m==WM_ERASEBKGND)return 1;
  if(m==WM_DRAWITEM){drawButton((DRAWITEMSTRUCT*)l);return TRUE;}
  if(m==WM_PAINT){PAINTSTRUCT ps;auto target=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);if(logSurface.size(target,r.right,r.bottom)){auto dc=logSurface.dc;FillRect(dc,&r,bg);{Gdiplus::Graphics g(dc);if(banner)g.DrawImage(banner,Gdiplus::Rect(0,0,800,80));}auto old=SelectObject(dc,titleFont);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,240,250));RECT t{24,22,760,70};DrawTextW(dc,pvlang::english?L"Diagnostics / Device health":L"Diagnóstico / Saúde do aparelho",-1,&t,DT_SINGLELINE);SelectObject(dc,old);logSurface.present(target,ps.rcPaint);}EndPaint(h,&ps);return 0;}

  if (m == WM_DESTROY) {
    logEdit = nullptr;
    logWindow = nullptr;
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}
void showLogs() {
  if (logWindow) {
    ShowWindow(logWindow, SW_RESTORE);
    SetForegroundWindow(logWindow);
    return;
  }
  WNDCLASSW c{};
  c.lpfnWndProc = LogProc;
  c.hInstance = instance;
  c.lpszClassName = L"PapelVivo.Logs";
  c.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  c.hbrBackground = bg;
  c.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
  RegisterClassW(&c);
  RECT r{0, 0, 800, 544};
  AdjustWindowRect(&r, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);
  logWindow =
      CreateWindowW(c.lpszClassName, pvlang::english?L"Papel Vivo — Diagnostics":L"Papel Vivo — Diagnóstico",
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_CLIPCHILDREN,
                    CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                    r.bottom - r.top, window, nullptr, instance, nullptr);
}
void initBanner() {
  HRSRC res = FindResourceW(instance, MAKEINTRESOURCEW(102), RT_RCDATA);
  if (!res)
    return;
  DWORD size = SizeofResource(instance, res);
  HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, size);
  if (!mem)
    return;
  void *dest = GlobalLock(mem);
  memcpy(dest, LockResource(LoadResource(instance, res)), size);
  GlobalUnlock(mem);
  IStream *stream = nullptr;
  if (SUCCEEDED(CreateStreamOnHGlobal(mem, TRUE, &stream))) {
    banner = Gdiplus::Bitmap::FromStream(stream);
    bannerStream = stream;
  } else
    GlobalFree(mem);
}
LRESULT CALLBACK ButtonMotion(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR,
                              DWORD_PTR) {
  if (m == WM_MOUSEMOVE && !GetPropW(h, L"hot")) {
    SetPropW(h, L"hot", (HANDLE)1);
    TRACKMOUSEEVENT t{sizeof(t), TME_LEAVE, h, 0};
    TrackMouseEvent(&t);
    BOOL anim = TRUE;
    SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &anim, 0);
    if (anim)
      SetTimer(h, 88, 20, nullptr);
    else {
      SetPropW(h, L"fade", (HANDLE)100);
      InvalidateRect(h, nullptr, FALSE);
    }
  }
  if (m == WM_MOUSELEAVE) {
    RemovePropW(h, L"hot");
    BOOL anim = TRUE;
    SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &anim, 0);
    if (anim)
      SetTimer(h, 88, 20, nullptr);
    else {
      RemovePropW(h, L"fade");
      InvalidateRect(h, nullptr, FALSE);
    }
  }
  if (m == WM_TIMER && w == 88) {
    int a = (int)(INT_PTR)GetPropW(h, L"fade"),
        target = GetPropW(h, L"hot") ? 100 : 0;
    a = target > a ? std::min(target, a + 20) : std::max(target, a - 20);
    SetPropW(h, L"fade", (HANDLE)(INT_PTR)a);
    InvalidateRect(h, nullptr, FALSE);
    if (a == target)
      KillTimer(h, 88);
    return 0;
  }
  if (m == WM_NCDESTROY) {
    KillTimer(h, 88);
    RemovePropW(h, L"fade");
    RemovePropW(h, L"hot");
    RemoveWindowSubclass(h, ButtonMotion, 1);
  }
  return DefSubclassProc(h, m, w, l);
}
void drawButton(DRAWITEMSTRUCT *d) {
 if(!d||d->CtlType!=ODT_BUTTON)return;
 int W=d->rcItem.right-d->rcItem.left,H=d->rcItem.bottom-d->rcItem.top;
 if(!buttonSurface.size(d->hDC,W,H))return;
 auto dc=buttonSurface.dc;RECT r{0,0,W,H};FillRect(dc,&r,bg);
 bool disabled=(d->itemState&ODS_DISABLED)!=0;int a=(int)(INT_PTR)GetPropW(d->hwndItem,L"fade");
 {using namespace Gdiplus;Graphics g(dc);g.SetSmoothingMode(SmoothingModeAntiAlias);
 GraphicsPath shape;float x=.5f,y=.5f,w=float(W)-1,h=float(H)-1,rad=14;
 shape.AddArc(x,y,rad,rad,180,90);shape.AddArc(x+w-rad,y,rad,rad,270,90);shape.AddArc(x+w-rad,y+h-rad,rad,rad,0,90);shape.AddArc(x,y+h-rad,rad,rad,90,90);shape.CloseFigure();
 int R=48,G=37,B=67;switch(d->CtlID){case ID_APPLY:case 7010:R=179;G=38;B=108;break;case ID_PREVIEW:R=25;G=97;B=104;break;case ID_STOP:case 7012:R=104;G=39;B=66;break;case ID_GALLERY:R=58;G=64;B=126;break;case ID_REMOVE_BG:R=101;G=50;B=139;break;}
 if(disabled){R=34;G=29;B=43;a=0;}if(d->itemState&ODS_SELECTED){R=std::max(0,R-15);G=std::max(0,G-15);B=std::max(0,B-15);}
 LinearGradientBrush fill(Point(0,0),Point(0,H),Color(255,R+18+a/10,G+18+a/10,B+18+a/10),Color(255,R,G,B));g.FillPath(&fill,&shape);Pen border(Color(255,disabled?70:151,disabled?62:123,disabled?80:175),1.f);g.DrawPath(&border,&shape);
 if(auto icon=uiIcon(iconForButton(d->CtlID)))g.DrawImage(icon,Rect(12,(H-20)/2,20,20));}
 wchar_t text[160]{};GetWindowTextW(d->hwndItem,text,160);auto old=SelectObject(dc,normalFont);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,disabled?RGB(144,134,157):RGB(249,245,255));r.left=38;r.right-=8;DrawTextW(dc,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);if(d->itemState&ODS_FOCUS){RECT f{4,4,W-4,H-4};DrawFocusRect(dc,&f);}SelectObject(dc,old);RECT output{0,0,W,H};buttonSurface.present(d->hDC,output);
}
int bannerOffset=0;
void paintPanel(HDC dc){using namespace Gdiplus;RECT r{0,0,uiWidth,uiHeight};FillRect(dc,&r,bg);int height=std::max(180,uiHeight-480);bool placeholder=false;
 {Graphics g(dc);g.SetInterpolationMode(InterpolationModeHighQualityBicubic);if(banner)g.DrawImage(banner,Rect(-8+bannerOffset,0,uiWidth+16,160));SolidBrush surface(Color(255,29,22,38));g.FillRectangle(&surface,24,352,uiWidth-360,height);g.FillRectangle(&surface,uiWidth-320,352,296,std::max(215,uiHeight-480));
 if(!preview||!playing||interactiveMove){auto file=posterFileFor(selected);placeholder=file.empty();if(!file.empty()){Bitmap image(file.c_str());if(image.GetLastStatus()==Ok&&image.GetWidth()&&image.GetHeight()){float scale=std::min(float(uiWidth-360)/image.GetWidth(),float(height)/image.GetHeight());float w=image.GetWidth()*scale,h=image.GetHeight()*scale;g.DrawImage(&image,RectF(24+(uiWidth-360-w)/2,352+(height-h)/2,w,h));}else placeholder=true;}}}
 auto old=SelectObject(dc,titleFont);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,240,248));RECT title{30,42,720,90};DrawTextW(dc,pvlang::english?L"Your desktop, alive.":L"Seu desktop, vivo.",-1,&title,DT_SINGLELINE);SelectObject(dc,normalFont);RECT version{32,16,650,38};DrawTextW(dc,L"PAPEL VIVO   /   2.0.1",-1,&version,DT_SINGLELINE);RECT sub{32,105,750,147};DrawTextW(dc,pvlang::english?L"Local videos. A lighter desktop. Your original wallpaper stays intact.":L"Vídeos locais. Um desktop mais leve. Seu wallpaper original fica intacto.",-1,&sub,DT_WORDBREAK);
 RECT label{24,333,uiWidth-350,352};DrawTextW(dc,pvlang::english?L"SELECTED MEDIA / PREVIEW":L"MÍDIA SELECIONADA / PRÉVIA",-1,&label,DT_SINGLELINE);
 if(placeholder){RECT text{60,400,uiWidth-390,540};DrawTextW(dc,pvlang::english?L"Choose a video or image.\nNo wallpaper is running until you press Apply.":L"Escolha um vídeo ou imagem.\nNenhum wallpaper inicia antes de você clicar em Aplicar.",-1,&text,DT_WORDBREAK);}
 int side=uiWidth-310;RECT f{side,361,uiWidth-24,382};DrawTextW(dc,pvlang::english?L"Presentation limit":L"Limite de apresentação",-1,&f,DT_SINGLELINE);RECT vol{side,475,uiWidth-24,500};DrawTextW(dc,pvlang::english?L"Volume (0 = muted)":L"Volume (0 = sem áudio)",-1,&vol,DT_SINGLELINE);RECT health{side,540,uiWidth-24,561};DrawTextW(dc,pvlang::english?L"LIVE PERFORMANCE":L"PERFORMANCE AO VIVO",-1,&health,DT_SINGLELINE);SelectObject(dc,old);
}

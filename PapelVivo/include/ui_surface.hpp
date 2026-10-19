#pragma once
// GDI UI only: no change to the libmpv render context or its presentation policy.
struct UiSurface {
 HDC dc=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ original=nullptr;int width=0,height=0;
 UiSurface()=default;UiSurface(const UiSurface&)=delete;
 ~UiSurface(){reset();}
 void reset(){if(dc&&original)SelectObject(dc,original);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);dc=nullptr;bitmap=nullptr;original=nullptr;width=height=0;}
 bool size(HDC target,int w,int h){if(w<=0||h<=0||w>4096||h>4096)return false;if(dc&&width==w&&height==h)return true;HDC next=CreateCompatibleDC(target);if(!next)return false;HBITMAP image=CreateCompatibleBitmap(target,w,h);if(!image){DeleteDC(next);return false;}reset();dc=next;bitmap=image;original=SelectObject(dc,bitmap);width=w;height=h;return true;}
 void present(HDC target,const RECT&r,int sourceX=0,int sourceY=0){if(!dc)return;BOOL composed=FALSE;static ULONGLONG last=0;auto now=GetTickCount64();if(now-last>=8&&SUCCEEDED(DwmIsCompositionEnabled(&composed))&&composed){DwmFlush();last=GetTickCount64();}BitBlt(target,r.left,r.top,r.right-r.left,r.bottom-r.top,dc,sourceX+r.left,sourceY+r.top,SRCCOPY);}
};
UiSurface mainSurface,gallerySurface,dialogSurface,buttonSurface;
int uiWidth=960,uiHeight=900,scrollX=0,scrollY=0;bool arranging=false,interactiveMove=false;
HDWP controlBatch=nullptr;
void moveControl(HWND child,int x,int y,int w,int h){if(!IsWindow(child)||w<1||h<1)return;RECT old{};GetWindowRect(child,&old);MapWindowPoints(nullptr,GetParent(child),(POINT*)&old,2);if(old.left==x&&old.top==y&&old.right-old.left==w&&old.bottom-old.top==h)return;if(controlBatch){controlBatch=DeferWindowPos(controlBatch,child,nullptr,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS);}else SetWindowPos(child,nullptr,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS);}

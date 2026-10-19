#pragma once
#include <string>
#include <vector>
#include <windows.h>
namespace pvlang {
inline bool english = false;
inline std::wstring text(const std::wstring &in) {
  if (!english)
    return in;
  static const std::vector<std::pair<std::wstring, std::wstring>> catalog = {
      {L"Falha ao inicializar mpv.", L"Failed to initialize mpv."},
      {L"Não foi possível carregar libmpv-2.dll x64. Reinstale o aplicativo e "
       L"confira a quarentena do antivírus sem desativar a proteção. Código "
       L"Windows: ",
       L"Could not load 64-bit libmpv-2.dll. Reinstall the app and check "
       L"antivirus quarantine without disabling protection. Windows code: "},
      {L"A libmpv não contém a API necessária. Reinstale a biblioteca "
       L"compatível pelo instalador.",
       L"libmpv does not expose the required API. Reinstall the compatible "
       L"library with setup."},
      {L"Papel Vivo 2.0.1 — kz\n\nEscolha um vídeo local e teste a prévia antes de aplicar ao desktop. Use Pausar/Continuar para controlar a reprodução e Voltar ao original para restaurar sua área de trabalho.\n\nO limite de 30/60 FPS e o desempenho dependem do vídeo e do computador. O modo alternativo pode cobrir os ícones.\n\nWindows 10/11 de 64 bits. Uso offline após o download. Consulte o guia do usuário e as licenças na pasta do aplicativo.", L"Papel Vivo 2.0.1 — kz\n\nChoose a local video and test Preview before applying it to your desktop. Use Pause/Resume to control playback and Restore original to restore your desktop.\n\nThe 30/60 FPS limit and performance depend on the video and your computer. Alternative mode may cover desktop icons.\n\n64-bit Windows 10/11. Offline use after download. See the user guide and licenses in the application folder."},
      {L"Reprodução: ", L"Playback: "},
      {L"por software", L"software decoding"},
      {L"aceleração de vídeo ativa", L"video acceleration active"},
      {L"Para aplicar o idioma, é necessário reiniciar o "
       L"aplicativo.\n\nReiniciar agora? O wallpaper será interrompido; "
       L"reaplique-o depois de abrir.\n\nSim: reiniciar agora. Não: aplicar na "
       L"próxima abertura. Cancelar: não mudar.",
       L"The app must restart to apply the language.\n\nRestart now? The "
       L"wallpaper will stop; apply it again after opening.\n\nYes: restart "
       L"now. No: apply on next launch. Cancel: do not change."},
      {L"Esse vídeo é pesado. Quer converter pra 1080p (mais leve)?\n\nSim: "
       L"cria uma cópia H.264/30 FPS e aplica. O original é preservado.\nNão: "
       L"mantém o original (pode sobrecarregar seu PC).\nCancelar: não aplica "
       L"este arquivo.",
       L"This video is heavy. Convert it to a lighter 1080p copy?\n\nYes: "
       L"creates a H.264/30 FPS copy and applies it. The original is "
       L"preserved.\nNo: keeps the original (it may overload your "
       L"PC).\nCancel: does not apply this file."},
      {L"Seu Windows bloqueou. Usando modo de sobreposição (não "
       L"interativo).\n\nEste modo pode cobrir os ícones e não é o encaixe "
       L"padrão. Use Voltar ao original para desfazer.",
       L"Windows blocked the standard method. Using a non-interactive "
       L"overlay.\n\nThis mode may cover the icons and is not standard "
       L"attachment. Use Restore original to undo."},
      {L"O dispositivo gráfico foi perdido ou houve erro ao apresentar vídeo. "
       L"O original foi preservado. Atualize o driver e aplique novamente.",
       L"The graphics device was lost or video presentation failed. The "
       L"original was preserved. Update the driver and apply again."},
      {L"Não foi possível reproduzir este vídeo. Ele pode estar corrompido, "
       L"sem codec suportado ou inacessível. Código mpv: ",
       L"Could not play this video. It may be corrupted, unsupported or "
       L"inaccessible. mpv code: "},
      {L"O Explorer mudou. O vídeo foi parado com segurança; aplique novamente "
       L"quando o desktop estiver disponível.",
       L"Explorer changed. Video was safely stopped; apply it again when the "
       L"desktop is available."},
      {L"A cópia não passou na verificação ou não pôde ser salva. O original "
       L"foi preservado.",
       L"The copy could not be verified or saved. The original was preserved."},
      {L"Pronto para dar vida à sua tela.\n\nEscolha um vídeo ou teste a Brisa "
       L"Azul incluída.",
       L"Ready to bring your screen to life.\n\nChoose a video or try the "
       L"included Brisa Azul."},
      {L"Convertendo para 1080p/30. A interface continua ativa; Parar encerra "
       L"e cancela.",
       L"Converting to 1080p/30. The interface remains active; Stop exits and "
       L"cancels."},
      {L"Vídeos locais. Sem navegador embutido.\nO papel de parede original "
       L"fica intacto.",
       L"Local videos. No embedded browser.\nYour original wallpaper stays "
       L"intact."},
      {L"\n\nO registro real está em Diagnóstico. Você pode copiá-lo para "
       L"relatar o erro.",
       L"\n\nThe actual record is in Diagnostics. You can copy it to report "
       L"the error."},
      {L"\r\nVídeo acima de 1080p: prefira uma cópia H.264 1080p/30 para "
       L"reduzir carga.",
       L"\r\nVideo above 1080p: prefer a H.264 1080p/30 copy to reduce load."},
      {L"O Windows recusou o modo alternativo. Seu wallpaper original foi "
       L"preservado.",
       L"Windows refused alternative mode. Your original wallpaper was "
       L"preserved."},
      {L"Registros reais do aparelho. Revise caminhos pessoais antes de "
       L"compartilhar.",
       L"Actual device logs. Review personal paths before sharing."},
      {L"Verificando resolução antes de abrir o vídeo... Parar encerra o "
       L"aplicativo.",
       L"Checking resolution before opening the video... Stop exits the app."},
      {L"Não foi possível posicionar o modo alternativo. O original foi "
       L"preservado.",
       L"Could not position alternative mode. The original was preserved."},
      {L"Escolha um arquivo local MP4, WebM, MKV, AVI ou MOV que exista no "
       L"disco.",
       L"Choose an existing local MP4, WebM, MKV, AVI or MOV file."},
      {L"O modo alternativo não pôde ser exibido. Seu original foi preservado.",
       L"Alternative mode could not be displayed. Your original was "
       L"preserved."},
      {L"O vídeo não começou a renderizar. Não foi confirmado como aplicado.",
       L"Video rendering did not start. Application was not confirmed."},
      {L"Seu Windows bloqueou. Usando modo de sobreposição (não interativo).",
       L"Windows blocked the standard method. Using a non-interactive "
       L"overlay."},
      {L"Convertendo para 1080p/30. Parar encerra e cancela com segurança.",
       L"Converting to 1080p/30. Stop safely exits and cancels."},
      {L"Não foi possível verificar a resolução antes de abrir o vídeo. ",
       L"Could not check the resolution before opening the video. "},
      {L"Não foi possível criar a superfície de vídeo. Código Windows: ",
       L"Could not create the video surface. Windows code: "},
      {L"Parado. O papel de parede estático do Windows está visível.",
       L"Stopped. The Windows static wallpaper is visible."},
      {L"Aplicado no desktop.\nMinimize as janelas (Win+D) para ver.",
       L"Applied to the desktop.\nMinimize windows (Win+D) to see it."},
      {L"Não foi possível reiniciar o aplicativo. Código Windows: ",
       L"Could not restart the app. Windows code: "},
      {L"Não foi possível localizar a pasta local de conversões.",
       L"Could not locate the local conversion folder."},
      {L"O mpv recusou abrir o arquivo selecionado. Código mpv: ",
       L"mpv refused to open the selected file. mpv code: "},
      {L"Vídeo selecionado. Aplique no desktop ou pré-visualize.",
       L"Video selected. Apply it to the desktop or preview it."},
      {L"Vídeo verificado. Aplique no desktop ou pré-visualize.",
       L"Video checked. Apply it to the desktop or preview it."},
      {L"Pré-visualização em janela — ainda não é o wallpaper.",
       L"Window preview — not the desktop wallpaper yet."},
      {L"Preparando vídeo e verificando encaixe no desktop...",
       L"Preparing video and checking desktop attachment..."},
      {L"Pausado automaticamente: aplicativo em tela cheia.",
       L"Automatically paused: a full-screen app is open."},
      {L"Não foi possível salvar o idioma. Código Windows: ",
       L"Could not save the language. Windows code: "},
      {L"Não foi possível abrir o painel. Código Windows: ",
       L"Could not open the panel. Windows code: "},
      {L"Idioma salvo. Será aplicado na próxima abertura.",
       L"Language saved. It will be applied on the next launch."},
      {L"A conversão falhou; o original foi preservado. ",
       L"Conversion failed; the original was preserved. "},
      {L"Escolha um vídeo local válido antes de aplicar.",
       L"Choose a valid local video before applying."},
      {L"Não foi possível criar eventos de renderização.",
       L"Could not create rendering events."},
      {L"Operação cancelada. O original foi preservado.",
       L"Operation canceled. The original was preserved."},
      {L"Papel Vivo 2.0.1 | kz | Win32 x64 | processo ",
       L"Papel Vivo 2.0.1 | kz | Win32 x64 | process "},
      {L"Pausado por você. Use Continuar para retomar.",
       L"Paused by you. Use Resume to continue."},
      {L"Seu papel de parede original está preservado.",
       L"Your original wallpaper is preserved."},
      {L"Este arquivo não contém um vídeo compatível.",
       L"This file does not contain a compatible video."},
      {L"Prévia pausada: janela oculta ou minimizada.",
       L"Preview paused: window hidden or minimized."},
      {L"Não foi possível criar a pasta Convertidos.",
       L"Could not create the converted videos folder."},
      {L"Recomendado: H.264, 720p/1080p, até 30 FPS.",
       L"Recommended: H.264, 720p/1080p, up to 30 FPS."},
      {L"Não foi possível criar o renderer OpenGL: ",
       L"Could not create the OpenGL renderer: "},
      {L"SEU VÍDEO  /  MP4 · WEBM · MKV · AVI · MOV",
       L"YOUR VIDEO  /  MP4 · WEBM · MKV · AVI · MOV"},
      {L"Aplicação cancelada; nada foi convertido.",
       L"Application canceled; nothing was converted."},
      {L"Não foi possível criar o nome da cópia.",
       L"Could not create a name for the copy."},
      {L"Papel Vivo — vídeo acima de 1920x1080",
       L"Papel Vivo — video above 1920x1080"},
      {L"Pausado: tela desligada ou suspensão.",
       L"Paused: display off or sleep."},
      {L". Use a DLL e um driver compatíveis.",
       L". Use a compatible DLL and driver."},
      {L"\nCtrl+C copia esta mensagem. Logs: ",
       L"\nCtrl+C copies this message. Logs: "},
      {L"Não foi possível iniciar a libmpv.", L"Could not initialize libmpv."},
      {L"Papel Vivo — vídeo leve no desktop",
       L"Papel Vivo — lightweight desktop video"},
      {L"Papel Vivo — diagnóstico copiável",
       L"Papel Vivo — copyable diagnostics"},
      {L"Escolher vídeo para o Papel Vivo", L"Choose a video for Papel Vivo"},
      {L"Nenhum vídeo sendo decodificado.", L"No video is being decoded."},
      {L"Papel Vivo 2.0.1 | autor kz\r\n", L"Papel Vivo 2.0.1 | author kz\r\n"},
      {L"Papel Vivo — modo alternativo", L"Papel Vivo — alternative mode"},
      {L"Arquivos, README e licenças", L"Files, README and licenses"},
      {L"Aplicado atrás dos ícones.", L"Applied behind the icons."},
      {L"CPU (sem aceleração ativa)", L"CPU (no active acceleration)"},
      {L"Pausado: sessão bloqueada.", L"Paused: session locked."},
      {L"Idioma: Português (Brasil)", L"Language: Português (Brasil)"},
      {L" FPS apresentados / teto ", L" FPS presented / limit "},
      {L" MB  |  Memória privada: ", L" MB  |  Private memory: "},
      {L"Diagnóstico / copiar logs", L"Diagnostics / copy logs"},
      {L"Nenhum vídeo selecionado", L"No video selected"},
      {L"Opção mpv incompatível: ", L"Incompatible mpv option: "},
      {L"  |  CPU do processo: ", L"  |  Process CPU: "},
      {L"Limite de apresentação", L"Presentation limit"},
      {L"Volume (0 = sem áudio)", L"Volume (0 = silent)"},
      {L"Pausar em tela cheia", L"Pause in full screen"},
      {L"Abrir pasta de logs", L"Open log folder"},
      {L"Papel Vivo — idioma", L"Papel Vivo — language"},
      {L"Aplicar ao desktop", L"Apply to desktop"},
      {L"Sobre e desempenho", L"About and performance"},
      {L"Sobre o Papel Vivo", L"About Papel Vivo"},
      {L"Voltar ao original", L"Restore original"},
      {L"Seu desktop, vivo.", L"Your desktop, alive."},
      {L"Aplicar wallpaper", L"Apply wallpaper"},
      {L"Escolher vídeo...", L"Choose video..."},
      {L"Papel Vivo — erro", L"Papel Vivo — error"},
      {L"Abrir Papel Vivo", L"Open Papel Vivo"},
      {L"Configurações...", L"Settings..."},
      {L"Copiar relatório", L"Copy report"},
      {L"Modo alternativo", L"Alternative mode"},
      {L"Parar e encerrar", L"Stop and exit"},
      {L"RAM residente: ", L"Resident RAM: "},
      {L"Trocar vídeo...", L"Change video..."},
      {L"Idioma: English", L"Language: English"},
      {L"Pré-visualizar", L"Preview"},
      {L"Parar e sair", L"Stop and exit"},
      {L"indisponível", L"unavailable"},
      {L"Diagnóstico", L"Diagnostics"},
      {L"Continuar", L"Resume"},
      {L"Código ", L"Code "},
      {L"Pausar", L"Pause"},
      {L"Fechar", L"Close"},
      {L"INÍCIO", L"START"},
      {L"Idioma", L"Language"},
      {L"Sobre", L"About"},
      {L"ERRO", L"ERROR"},
      {L"Sair", L"Exit"},
      {L"AÇÃO", L"ACTION"},
  };
  std::wstring out = in;
  for (const auto &pair : catalog) {
    size_t at = 0;
    while ((at = out.find(pair.first, at)) != std::wstring::npos) {
      out.replace(at, pair.first.size(), pair.second);
      at += pair.second.size();
    }
  }
  return out;
}
inline bool edit(LPCWSTR cls) {
  return (ULONG_PTR)cls > 65535 && !_wcsicmp(cls, L"EDIT");
}
inline HWND create(DWORD ex, LPCWSTR cls, LPCWSTR label, DWORD style, int x,
                   int y, int w, int h, HWND parent, HMENU menu, HINSTANCE inst,
                   LPVOID param) {
  auto translated = text(label ? label : L"");
  return ::CreateWindowExW(ex, cls, edit(cls) ? label : translated.c_str(),
                           style, x, y, w, h, parent, menu, inst, param);
}
inline BOOL set(HWND h, LPCWSTR label) {
  wchar_t cls[80]{};
  ::GetClassNameW(h, cls, 80);
  auto translated = text(label ? label : L"");
  return ::SetWindowTextW(h, edit(cls) ? label : translated.c_str());
}
inline int message(HWND h, LPCWSTR body, LPCWSTR title, UINT flags) {
  auto b = text(body ? body : L""), t = text(title ? title : L"");
  return ::MessageBoxW(h, b.c_str(), t.c_str(), flags);
}
inline BOOL menu(HMENU m, UINT flags, UINT_PTR id, LPCWSTR label) {
  if (flags & (MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))
    return ::AppendMenuW(m, flags, id, label);
  auto t = text(label ? label : L"");
  return ::AppendMenuW(m, flags, id, t.c_str());
}
inline int draw(HDC dc, LPCWSTR label, int count, LPRECT rect, UINT flags) {
  auto t = text(label ? std::wstring(label, count < 0 ? wcslen(label) : count)
                      : L"");
  return ::DrawTextW(dc, t.c_str(), (int)t.size(), rect, flags);
}
} // namespace pvlang
#undef CreateWindowW
#define CreateWindowExW pvlang::create
#define CreateWindowW(cls, label, style, x, y, w, h, parent, menu, inst,       \
                      param)                                                   \
  pvlang::create(0, cls, label, style, x, y, w, h, parent, menu, inst, param)
#define SetWindowTextW pvlang::set
#define MessageBoxW pvlang::message
#define AppendMenuW pvlang::menu
#define DrawTextW pvlang::draw

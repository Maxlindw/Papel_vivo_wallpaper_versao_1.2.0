#include "policy.hpp"
#include <cstdlib>
#include <iostream>
int checks = 0;
void check(bool b, const char *m) {
    checks++;
    if (!b) {
        std::cerr << "FALHOU: " << m << '\n';
        std::exit(1);
    }
}
int main() {
    using namespace pv;
    Rect screen{0, 0, 1920, 1080};
    check(covers(screen, screen), "tela cheia exata");
    check(!covers({0, 0, 1920, 1040}, screen), "maximizado sem cobrir taskbar");
    check(covers({-1, -1, 1921, 1081}, screen), "bordas negativas");
    check(!covers({10, 0, 1920, 1080}, screen), "janela menor");
    check(covers({-1920, 0, 0, 1080}, {-1920, 0, 0, 1080}), "monitor com origem negativa");
    check(!covers({0, 0, 0, 0}, screen), "retangulo vazio");
    PausePolicy p;
    check(!p.paused(), "inicio sem pausa");
    p.sample(true, 0);
    check(p.paused(), "fullscreen pausa imediatamente");
    p.sample(false, 100);
    p.sample(false, 849);
    check(p.paused(), "debounce 749 ms");
    p.sample(false, 850);
    check(!p.paused(), "retoma depois de 750 ms");
    p.manual = true;
    p.sample(true, 900);
    p.sample(false, 1000);
    p.sample(false, 2000);
    check(p.paused(), "pausa manual preservada ao fechar jogo");
    p.manual = false;
    check(!p.paused(), "continuar");
    p.locked = true;
    check(p.paused(), "bloqueio");
    p.locked = false;
    p.displayOff = true;
    check(p.paused(), "tela apagada");
    p.displayOff = false;
    p.suspended = true;
    check(p.paused(), "suspensao");
    p.suspended = false;
    p.sample(true, 3000);
    p.automatic = false;
    check(!p.paused(), "autopausa desligada ignora fullscreen");
    p.manual = true;
    check(p.paused(), "manual independente de auto");
    p.manual = false;
    p.locked = true;
    check(p.paused(), "bloqueio independente de auto");
    FrameLimiter f;
    check(f.fps() == 30, "30 padrao");
    f.presented(1000);
    check(f.waitUs(1000) == 33334, "periodo arredondado para cima");
    check(f.waitUs(34334) == 0, "prazo cumprido");
    check(f.waitUs(100000) == 0, "sem atraso negativo");
    f.presented(100000);
    check(f.waitUs(100001) == 33333, "sem rajada para recuperar atraso");
    f.fps(60);
    check(f.waitUs(1) == 0, "alterar fps limpa prazo");
    f.presented(100);
    check(f.waitUs(100) == 16667, "60 fps");
    f.fps(144);
    check(f.fps() == 30, "entrada invalida usa 30");
    for (int rate : {30, 60}) {
        f.fps(rate);
        int64_t now = 0;
        int count = 0;
        while (now < 1000000) {
            now += f.waitUs(now);
            if (now >= 1000000)
                break;
            f.presented(now);
            count++;
            now++;
        }
        check(count <= rate, "teto em janela de um segundo");
    }
    std::cout << checks << " verificacoes aprovadas\n";
}

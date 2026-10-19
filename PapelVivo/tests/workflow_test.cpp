#include "workflow.hpp"
#include <cstdlib>
#include <iostream>
int checks = 0;
void check(bool b, const char *name) {
    ++checks;
    if (!b) {
        std::cerr << "FALHOU: " << name << '\n';
        std::exit(1);
    }
}
int main() {
    using namespace pv;
    check(dimensions("3584x2016\n")->heavy(), "3584x2016 pede conversao");
    check(!dimensions("1920x1080\r\n")->heavy(), "1080p nao pede conversao");
    check(dimensions("1080x1920")->heavy(), "vertical acima do limite");
    check(dimensions("1922x1080")->heavy(), "limite largura");
    check(dimensions("1920x1082")->heavy(), "limite altura");
    check(!dimensions("1280x720")->heavy(), "720p preservado");
    for (const char *text : {"", "0x1080", "1920x0", "1920x", "x1080", "foo", "1920x1080 lixo",
                             "999999999999999x1080", "-1x20", "100001x10"})
        check(!dimensions(text), "metadados invalidos falham fechados");
    check(!usesRaisedDesktop(0), "sem NOREDIRECTION usa legado");
    check(!usesRaisedDesktop(0x00080000u), "LAYERED sozinho nao escolhe modo novo");
    check(usesRaisedDesktop(0x00200000u), "NOREDIRECTION seleciona raised desktop");
    check(usesRaisedDesktop(0x00200080u), "flag independe de outros estilos");
    check(!usesRaisedDesktop(0x00000080u), "sem flag nao presume pelo build");
    check(quoteArg(L"") == L"\"\"", "argumento vazio");
    check(quoteArg(L"C:\\Vídeos\\ação & som.mp4") == L"\"C:\\Vídeos\\ação & som.mp4\"",
          "espacos e metacaracteres nao usam shell");
    check(quoteArg(L"a\"b") == L"\"a\\\"b\"", "aspas escapadas");
    check(quoteArg(L"C:\\pasta\\") == L"\"C:\\pasta\\\\\"", "barra final preservada");
    std::cout << checks << " verificacoes de correcao aprovadas\n";
}

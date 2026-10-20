# 🎨 Papel Vivo — Wallpaper Animado para PC Fraco

> **O projeto que nasceu da dor de ter um PC batata.**

O Papel Vivo é um aplicativo gratuito, feito em C++, que roda vídeos animados como papel de parede no Windows. Ele foi desenhado especificamente para **PCs fracos** (como Intel i3 com 8GB de RAM), usando hardware decoding (d3d11va) para consumir apenas **~5% de CPU**.

## 🚀 Baixe a Versão Final (v2.0.1)
O instalador oficial e o manual podem ser baixados no itch.io:
👉 **[Baixar o Papel Vivo no itch.io](https://kyzinh.itch.io/papel-vivo-wallpaper-animado-leve-para-pc-fraco)**

## 📖 A História deste Projeto
Eu sou o **Kz**, um desenvolvedor solo brasileiro. Eu criei o Papel Vivo porque eu não podia usar o Wallpaper Engine (que custa caro e pesa muito). Eu queria algo em português, leve e gratuito. 

Passei dias e noites quebrando a cabeça, testando 8 versões diferentes (da RC1 até a 2.0.1). No caminho, enfrentei bugs de física, processos que não fechavam e até um Estúdio de Criação de Cenas que ficou grande demais para eu manter sozinho.

Esta é a versão final deste ciclo. O Estúdio foi congelado, mas o Player está estável, leve e pronto para uso. O projeto não morreu; ele só parou de ser atualizado por falta de apoio e tempo. Se alguém quiser continuar, o código-fonte está todo aqui.

## 📦 O que tem nesta versão? (v2.0.1)
- **Leveza:** Apenas 141 MB.
- **Galeria:** Salva os wallpapers que você já usou (com opção de renomear e excluir).
- **Tira-Fundo Offline:** Ferramenta integrada para remover fundo de imagens.
- **Fim dos Processos Fantasmas:** O app fecha de verdade.
- **Interface Estável:** Sem piscar, com a ajuda da nossa personagem de anime.

## 🛠️ Tecnologias
- **Linguagem:** C++ (Win32 API, Direct3D 11)
- **Motor de Vídeo:** libmpv
- **Ferramentas:** FFmpeg, rembg (para tira-fundo)
- **Foco:** Performance e leveza em hardware antigo.

## 📜 Licença
O núcleo do projeto é open-source sob a licença MIT. As bibliotecas de terceiros (libmpv, FFmpeg) mantêm suas licenças originais (LGPL/GPL).

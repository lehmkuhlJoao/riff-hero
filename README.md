# Riff Hero

Projeto acadêmico desenvolvido para a disciplina de Estrutura de Dados, o **Riff Hero** é um jogo de ritmo inspirado no Guitar Hero. O jogo foi construído do zero utilizando C++ moderno, com foco em desempenho, portabilidade e efeitos visuais avançados.

## Sobre o Jogo

- Suporte para dois jogadores simultâneos.
- Detecção precisa de acertos com margem de tolerância.
- Notas longas (sustain) com pontuação contínua.
- Sistema de partículas para feedback visual.
- Fundo animado utilizando shaders em GLSL.
- Leitura de arquivos `.chart` personalizados.
- Suporte completo a UTF-8 para nomes com acentuação.
- Atualmente compilado para **Windows**, mas com potencial para ser portado para Linux e macOS.

## Tecnologias Utilizadas

### C++20

Optamos por utilizar o C++20 devido às suas melhorias significativas em segurança e legibilidade de código, como:

- `std::ranges` e `std::views` para manipulação eficiente de coleções.
- `std::optional` para tratamento seguro de valores opcionais.
- `std::numbers` para constantes matemáticas padronizadas.
- `std::string_view` para manipulação eficiente de strings.

Essas funcionalidades modernas permitiram um código mais limpo, seguro e performático.

### SFML 3.0

A **Simple and Fast Multimedia Library (SFML)** é uma biblioteca multimídia multiplataforma escrita em C++. Utilizamos a versão 3.0, que trouxe diversas melhorias:

- Atualização para suporte e uso de C++17.
- Substituição do OpenAL por miniaudio, simplificando o gerenciamento de áudio.
- Novas APIs de tratamento de eventos mais robustas.
- Suporte a scissor e stencil testing.
- Melhorias na modularidade e suporte a UTF-8.

A SFML foi escolhida por sua simplicidade e eficiência na manipulação de gráficos, áudio e entrada de usuário, sem ser uma biblioteca de interface gráfica como o Swing.

### OpenGL e GLSL

Para efeitos visuais avançados, como o fundo animado com cores listradas, utilizamos **OpenGL** diretamente com shaders escritos em **GLSL**. A SFML permite a integração nativa com OpenGL, facilitando a criação de efeitos personalizados que são executados diretamente na GPU, garantindo alta performance.

Os shaders utilizados estão localizados na pasta `Assets/`:

- `shader_notas.vsh/.fsh`: Shaders para renderização das notas.
- `shader_fundo.vsh/.fsh`: Shader para o fundo animado.

### CMake

Para automatizar o processo de build, utilizamos o **CMake**, uma ferramenta moderna e multiplataforma que facilita a compilação e organização do projeto. Com o CMake, evitamos a necessidade de compilar manualmente, tornando o processo mais prático e eficiente.

## Estrutura do Projeto

```
riff-hero-cpp/
├── src/ # Código-fonte principal
│ ├── main.cpp
├── notes.chart # Mapa de notas da música
├── song.ogg # Música correspondente ao chart
├── fonte.ttf # Fonte utilizada no jogo
├── hit.ogg # Som de acerto
├── branco.png # Textura auxiliar
├── background.png # Fundo do jogo
├── shader_notas.vsh/.fsh # Shaders de notas (GLSL)
├── shader_fundo.vsh/.fsh # Shader de fundo animado
├── CMakeLists.txt # Script de build com CMake
└── README.md # Este documento
```

## Controles

| Jogador | Teclas |
|--------|--------|
| J1     | A, S, D, F, G |
| J2     | J, K, L, ;, ' |
| Ambos  | Espaço = Iniciar / Reiniciar |

## Formato `.chart`

Utilizamos o formato `.chart`, popular na comunidade de fãs do Guitar Hero, para representar os mapas das músicas. Tentamos implementar detecção automática de notas, mas a precisão era baixa. A escolha por `.chart` permite mapas feitos por humanos, com resultados muito melhores.

Esses arquivos incluem:

- Metadados (nome, artista, álbum).
- Notas (tick, pista, duração).
- Mudança de tempo (BPM).
- Assinaturas de compasso.

A leitura do `.chart` foi implementada com um parser próprio.

## Controles

| Jogador | Teclas                |
|---------|-----------------------|
| J1      | A, S, D, F, G         |
| J2      | J, K, L, ;, '         |
| Ambos   | Espaço = Iniciar/Reiniciar |

## Equipe e Tarefas

- **Vinícius Schroeder** & **Gabriel Rech Brand** — Desenvolvimento de algorítmos com estrutura de dados, lógica e regras do jogo.
- **Gabriel Rech Brand & João Vitor Broering Lehmkuhl** — Interface do jogo.
- **André de Oliveira da Silva & Luiz Eduardo Ramos Maier** — Documentação e análise de complexidade.
- **Luiz Eduardo Ramos Maier** — Organização do projeto.

## Licença e Direitos Autorais

Este projeto é de uso exclusivamente acadêmico. Todos os charts utilizados foram criados por fãs e mantêm seus devidos créditos no aplicativo.
**Não possuímos fins comerciais nem redistribuímos conteúdo protegido.**

---

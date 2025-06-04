/**
 * @file main.cpp
 * @brief Jogo de ritmo estilo Guitar Hero desenvolvido com SFML
 * @author Vinícius Schroeder, Gabriel Rech Brand, Luiz Eduardo Ramos Maier, André de Oliveira da Silva, João Vitor Broering Lehmkuhl
 * @date 2025
 * @version 1.0
 *
 * Este é um jogo de ritmo para dois jogadores onde os usuários devem pressionar
 * teclas no momento correto conforme as notas caem pela tela, sincronizadas
 * com a música de fundo.
 *
 * Características:
 * - Suporte para dois jogadores simultâneos
 * - Sistema de partículas para feedback visual
 * - Notas longas (sustain) com pontuação contínua
 * - Carregamento de charts no formato ".chart"
 * - Shaders para efeitos visuais aprimorados
 * - Sistema de tolerância para acertos
 * - Suporte completo a UTF-8/UTF-32 para acentos
 *
 * Controles:
 * - Jogador 1: "A", "S", "D", "F", "G"
 * - Jogador 2: "J", "K", "L", ";", "'"
 * - Espaço: Iniciar/Reiniciar jogo
 */

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <cmath>
#include <optional>
#include <set>
#include <random>
#include <numbers>
#include <array>
#include <string_view>
#include <ranges>
#include <codecvt>
#include <locale>

// ============================= CONSTANTES GLOBAIS =============================

// Configurações de arquivo e janela
constexpr auto CAMINHO_ARQUIVO_CHART = "notes.chart";
constexpr auto LARGURA_JANELA = 1200;
constexpr auto ALTURA_JANELA = 1200;

// Layout da interface
constexpr auto LARGURA_PAINEL_CENTRAL = 400;
constexpr auto LARGURA_BRASTEADO = (LARGURA_JANELA - LARGURA_PAINEL_CENTRAL) / 2;
constexpr auto NUMERO_PISTAS = 5;
constexpr auto LARGURA_PISTA = LARGURA_BRASTEADO / NUMERO_PISTAS;

// Configurações das notas
constexpr auto ALTURA_NOTA = 45;
constexpr auto Y_ZONA_ACERTO = ALTURA_JANELA - 100;
constexpr auto ALTURA_ZONA_ACERTO = 75;
constexpr auto VELOCIDADE_QUEDA_NOTA_PPS = 800.0f;

// Configurações de timing
constexpr auto FPS_JOGO = 165;
constexpr auto ATUALIZACAO_JOGO_MS = 1000 / FPS_JOGO;
constexpr auto TOLERANCIA_ACERTO_MS = 200L;
constexpr auto OFFSET_LATENCIA_AUDIO_SEC = 0.0;

// Configurações de partículas
constexpr auto PARTICULAS_POR_ACERTO = 8;
constexpr auto PARTICULAS_SUSTAIN = 2;
constexpr auto TEMPO_VIDA_PARTICULA_MIN_SEC = 0.2f;
constexpr auto TEMPO_VIDA_PARTICULA_MAX_SEC = 0.5f;
constexpr auto VELOCIDADE_PARTICULA_MIN = 50.f;
constexpr auto VELOCIDADE_PARTICULA_MAX = 150.f;
constexpr auto VELOCIDADE_PARTICULA_SUSTAIN_MAX = 50.f;
constexpr auto TAMANHO_PARTICULA = 16.f;
constexpr auto INTERVALO_SPAWN_PARTICULA_SUSTAIN = sf::seconds(0.08f);

// Configurações do painel de pontuação
constexpr auto ALTURA_PAINEL_PONTUACAO = 60.f;
constexpr auto OFFSET_Y_PAINEL_PONTUACAO = 5.f;

// Nomes de uniformes para shaders
const auto UNIFORM_RESOLUCAO = "Resolucao";
const auto UNIFORM_TEMPO = "Tempo";
const auto UNIFORM_LARGURA_RETANGULO = "LarguraRetangulo";
const auto UNIFORM_ALTURA_RETANGULO = "AlturaRetangulo";
const auto UNIFORM_TEXTURA = "texture";

// ============================= UTILITÁRIOS UTF-8/UTF-32 =============================

/**
 * @brief Converte string UTF-8 para sf::String (UTF-32)
 * @param utf8Str String em UTF-8
 * @return sf::String em UTF-32
 */
sf::String utf8ParaSfString(const std::string& utf8Str) {
    return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
}

/**
 * @brief Converte std::wstring para sf::String (UTF-32)
 * @param wideStr String wide
 * @return sf::String em UTF-32
 */
sf::String wstringParaSfString(const std::wstring& wideStr) {
    return sf::String(wideStr);
}

/**
 * @brief Converte sf::String para std::string UTF-8
 * @param sfStr sf::String em UTF-32
 * @return String em UTF-8
 */
std::string sfStringParaUtf8(const sf::String& sfStr) {
    std::string utf8Str;
    sf::Utf32::toUtf8(sfStr.begin(), sfStr.end(), std::back_inserter(utf8Str));
    return utf8Str;
}

/**
 * @brief Lê arquivo texto com suporte a UTF-8
 * @param caminhoArquivo Caminho para o arquivo
 * @return Conteúdo do arquivo em UTF-8
 */
std::string lerArquivoUtf8(const std::string& caminhoArquivo) {
    std::ifstream arquivo(caminhoArquivo, std::ios::binary);
    if (!arquivo.is_open()) {
        return "";
    }

    std::string conteudo((std::istreambuf_iterator<char>(arquivo)),
                         std::istreambuf_iterator<char>());
    arquivo.close();

    // Remove BOM UTF-8 se presente
    if (conteudo.size() >= 3 &&
        static_cast<unsigned char>(conteudo[0]) == 0xEF &&
        static_cast<unsigned char>(conteudo[1]) == 0xBB &&
        static_cast<unsigned char>(conteudo[2]) == 0xBF) {
        conteudo.erase(0, 3);
    }

    return conteudo;
}

// ============================= ESTRUTURAS DE DADOS =============================

/**
 * @brief Representa uma partícula do sistema de efeitos visuais
 */
struct Particula {
    sf::Vector2f posicao;
    sf::Vector2f velocidade;
    sf::Time tempoVida;
    sf::Color cor;
    sf::RectangleShape forma;
};

/**
 * @brief Representa um jogador com suas configurações e estado
 */
struct Jogador {
    int pontuacao = 0;
    std::map<sf::Keyboard::Key, int> mapeamentoTeclaPista;
    int offsetAreaJogadorX = 0;
    sf::String nome;
    std::set<sf::Keyboard::Key> teclasPresionadas;
    std::map<int, bool> pistaPermiteAcertoNotaCurta;

    /**
     * @brief Construtor do jogador
     * @param n Nome do jogador
     * @param offsetX Posição X da área do jogador na tela
     * @param mapaTeclas Mapeamento de teclas para pistas
     */
    Jogador(const std::string& n, const int offsetX, std::map<sf::Keyboard::Key, int> mapaTeclas)
        : nome(utf8ParaSfString(n)), offsetAreaJogadorX(offsetX), mapeamentoTeclaPista(std::move(mapaTeclas)) {
        for (const auto &[tecla, pista] : mapeamentoTeclaPista) {
            pistaPermiteAcertoNotaCurta[pista] = true;
        }
    }

    /**
     * @brief Adiciona pontos ao jogador
     * @param pontos Quantidade de pontos a adicionar
     */
    void adicionarPontuacao(const int pontos) { pontuacao += pontos; }
};

// ============================= NAMESPACE CHART =============================

/**
 * @brief Namespace contendo estruturas e classes relacionadas ao parsing de charts
 */
namespace Chart {
    /**
     * @brief Representa uma mudança de tempo no chart
     */
    struct MudancaTempo {
        int tick;
        int valorBruto;

        constexpr MudancaTempo(const int t, const int val) : tick(t), valorBruto(val) {}

        /**
         * @brief Obtém o BPM (Batidas Por Minuto)
         * @return BPM como double
         */
        [[nodiscard]] constexpr auto obterBPM() const -> double {
            return static_cast<double>(valorBruto) / 1000.0;
        }

        /**
         * @brief Obtém microssegundos por batida
         * @return Microssegundos por batida
         */
        [[nodiscard]] constexpr auto obterMicrossegundosPorBatida() const -> double {
            const auto bpm = obterBPM();
            if (bpm <= 0) return 0;
            return 60'000'000.0 / bpm;
        }
    };

    /**
     * @brief Representa uma assinatura de tempo no chart
     */
    struct AssinaturaTempo {
        int tick;
        int numerador;
        int denominador;

        constexpr AssinaturaTempo(const int t, const int n, const int d)
            : tick(t), numerador(n), denominador(d) {}
    };

    /**
     * @brief Representa uma nota no chart
     */
    struct NotaChart {
        int tick;
        int traste;
        int comprimento;

        constexpr NotaChart(const int t, const int tr, const int comp)
            : tick(t), traste(tr), comprimento(comp) {}
    };

    /**
     * @brief Contém todos os dados de um chart
     */
    struct DadosChart {
        sf::String nome, artista, streamMusica;
        sf::String criadorChart, album, ano, genero, tipoMidia;
        double offset = 0.0;
        int resolucao = 192;
        int dificuldade = 0;
        double inicioPreview = 0.0;
        double fimPreview = 0.0;
        sf::String jogador2;
        std::map<int, MudancaTempo> mudancasTempo;
        std::map<int, AssinaturaTempo> assinaturasTempo;
        std::vector<NotaChart> notas;
    };

    /**
     * @brief Parser para arquivos de chart
     */
    class ParserChart {
    public:
        /**
         * @brief Faz o parsing de um arquivo de chart
         * @param caminhoArquivo Caminho para o arquivo .chart
         * @return DadosChart opcional (std::nullopt se houver erro)
         */
        [[nodiscard]] static auto fazerParsingChart(const std::string &caminhoArquivo) -> std::optional<DadosChart> {
            DadosChart dadosChart;

            // Lê arquivo com suporte UTF-8
            const std::string conteudoUtf8 = lerArquivoUtf8(caminhoArquivo);
            if (conteudoUtf8.empty()) {
                std::cerr << "Erro: Não foi possível abrir o arquivo de chart: " << caminhoArquivo << "\n";
                return std::nullopt;
            }

            std::istringstream streamArquivo(conteudoUtf8);
            std::string linhaAtual, secaoAtual;
            const std::regex padraoSecao(R"(\[(.+)\])");
            const std::regex padraoChaveValor(R"(\s*(.+?)\s*=\s*(.+))");
            const std::regex padraoNota(R"((\d+)\s*=\s*N\s+(\d+)\s+(\d+))");
            const std::regex padraoTempo(R"((\d+)\s*=\s*B\s+(\d+))");
            const std::regex padraoAssinaturaTempo(R"((\d+)\s*=\s*TS\s+(\d+)(?:\s+(\d+))?)");
            std::smatch correspondencia;

            while (std::getline(streamArquivo, linhaAtual)) {
                // Remove espaços em branco no início e fim
                const auto primeiroChar = linhaAtual.find_first_not_of(" \t\r\n");
                if (primeiroChar == std::string::npos) {
                    linhaAtual.clear();
                } else {
                    linhaAtual.erase(0, primeiroChar);
                    const auto ultimoChar = linhaAtual.find_last_not_of(" \t\r\n");
                    if (ultimoChar != std::string::npos) {
                        linhaAtual.erase(ultimoChar + 1);
                    }
                }

                // Pula linhas vazias e comentários
                if (linhaAtual.empty() || linhaAtual.rfind("//", 0) == 0) {
                    continue;
                }

                // Identifica seções
                if (std::regex_match(linhaAtual, correspondencia, padraoSecao)) {
                    secaoAtual = correspondencia[1].str();
                    continue;
                }

                // Pula chaves
                if (linhaAtual == "{" || linhaAtual == "}") {
                    continue;
                }

                // Processa linha na seção atual
                if (!secaoAtual.empty()) {
                    processarLinhaNaSecao(dadosChart, secaoAtual, linhaAtual, padraoChaveValor,
                                        padraoNota, padraoTempo, padraoAssinaturaTempo);
                }
            }

            // Ordena notas por tick
            std::ranges::sort(dadosChart.notas, [](const auto &a, const auto &b) {
                return a.tick < b.tick;
            });

            return dadosChart;
        }

    private:
        /**
         * @brief Processa uma linha dentro de uma seção específica
         */
        static void processarLinhaNaSecao(DadosChart &chart, const std::string &secao,
                                        const std::string &linha, const std::regex &padraoCV,
                                        const std::regex &padraoN, const std::regex &padraoT,
                                        const std::regex &padraoAT) {
            std::smatch correspondencia;

            if (secao == "Song") {
                if (std::regex_match(linha, correspondencia, padraoCV)) {
                    auto chave = correspondencia[1].str();
                    auto valor = correspondencia[2].str();

                    // Remove aspas se presentes
                    if (valor.length() >= 2 && valor.front() == '"' && valor.back() == '"') {
                        valor = valor.substr(1, valor.length() - 2);
                    }

                    // Mapeia chaves para campos da estrutura (convertendo para sf::String)
                    if (chave == "Name") chart.nome = utf8ParaSfString(valor);
                    else if (chave == "Artist") chart.artista = utf8ParaSfString(valor);
                    else if (chave == "Charter") chart.criadorChart = utf8ParaSfString(valor);
                    else if (chave == "Album") chart.album = utf8ParaSfString(valor);
                    else if (chave == "Year") chart.ano = utf8ParaSfString(valor);
                    else if (chave == "Genre") chart.genero = utf8ParaSfString(valor);
                    else if (chave == "MediaType") chart.tipoMidia = utf8ParaSfString(valor);
                    else if (chave == "Player2") chart.jogador2 = utf8ParaSfString(valor);
                    else if (chave == "MusicStream") chart.streamMusica = utf8ParaSfString(valor);
                    else if (chave == "Offset") chart.offset = std::stod(valor);
                    else if (chave == "Resolution") chart.resolucao = std::stoi(valor);
                    else if (chave == "Difficulty") chart.dificuldade = std::stoi(valor);
                    else if (chave == "PreviewStart") chart.inicioPreview = std::stod(valor);
                    else if (chave == "PreviewEnd") chart.fimPreview = std::stod(valor);
                }
            }
            else if (secao == "SyncTrack") {
                if (std::regex_match(linha, correspondencia, padraoT)) {
                    chart.mudancasTempo.emplace(std::stoi(correspondencia[1].str()),
                                              MudancaTempo(std::stoi(correspondencia[1].str()),
                                                         std::stoi(correspondencia[2].str())));
                }
                else if (std::regex_match(linha, correspondencia, padraoAT)) {
                    auto denominador = correspondencia[3].matched ? std::stoi(correspondencia[3].str()) : 4;
                    if (denominador == 0) denominador = 4;
                    chart.assinaturasTempo.emplace(std::stoi(correspondencia[1].str()),
                                                 AssinaturaTempo(std::stoi(correspondencia[1].str()),
                                                               std::stoi(correspondencia[2].str()),
                                                               denominador));
                }
            }
            else if (secao == "ExpertSingle" || secao == "HardSingle" ||
                     secao == "MediumSingle" || secao == "EasySingle") {
                if (std::regex_match(linha, correspondencia, padraoN)) {
                    chart.notas.emplace_back(NotaChart(std::stoi(correspondencia[1].str()),
                                                     std::stoi(correspondencia[2].str()),
                                                     std::stoi(correspondencia[3].str())));
                }
            }
        }
    };

    /**
     * @brief Calculadora de tempo baseada em mudanças de tempo do chart
     */
    class CalculadoraTempo {
    private:
        const DadosChart &referenciaChart;
        std::vector<MudancaTempo> mudancasTempoOrdenadas;

    public:
        /**
         * @brief Construtor da calculadora de tempo
         * @param chart Referência para os dados do chart
         */
        explicit CalculadoraTempo(const DadosChart &chart) : referenciaChart(chart) {
            // Copia e ordena mudanças de tempo
            for (const auto &mudanca: referenciaChart.mudancasTempo | std::views::values) {
                mudancasTempoOrdenadas.push_back(mudanca);
            }

            std::ranges::sort(mudancasTempoOrdenadas, [](const auto &a, const auto &b) {
                return a.tick < b.tick;
            });

            // Garante que há uma mudança de tempo no tick 0
            if (mudancasTempoOrdenadas.empty() || mudancasTempoOrdenadas.front().tick != 0) {
                mudancasTempoOrdenadas.insert(mudancasTempoOrdenadas.begin(), MudancaTempo(0, 120000));
            }
        }

        /**
         * @brief Converte ticks para segundos
         * @param tickAlvo Tick a ser convertido
         * @return Tempo em segundos
         */
        [[nodiscard]] auto ticksParaSegundos(const int tickAlvo) const -> double {
            auto tempoEmSegundos = 0.0;
            auto tickAtual = 0;

            for (size_t i = 0; i < mudancasTempoOrdenadas.size(); ++i) {
                const auto &eventoTempoAtual = mudancasTempoOrdenadas[i];
                const auto proximoTickTempo = (i + 1 < mudancasTempoOrdenadas.size())
                                                 ? mudancasTempoOrdenadas[i + 1].tick
                                                 : std::numeric_limits<int>::max();
                const auto tickFinalSegmento = std::min(tickAlvo, proximoTickTempo);
                const auto ticksNoSegmento = tickFinalSegmento - tickAtual;

                if (ticksNoSegmento <= 0 && tickAlvo > 0) {
                    if (tickAtual >= tickAlvo) break;
                } else if (ticksNoSegmento < 0) {
                    break;
                }

                const auto microssegundosPorBatida = eventoTempoAtual.obterMicrossegundosPorBatida();
                if (referenciaChart.resolucao == 0) {
                    std::cerr << "Erro: Resolução do chart é 0. Não é possível calcular tempo a partir de ticks.\n";
                    return tempoEmSegundos + referenciaChart.offset;
                }

                const auto microssegundosPorTick = microssegundosPorBatida / static_cast<double>(referenciaChart.resolucao);
                const auto segundosPorTick = microssegundosPorTick / 1'000'000.0;
                const auto duracaoSegmentoSegundos = static_cast<double>(ticksNoSegmento) * segundosPorTick;

                tempoEmSegundos += duracaoSegmentoSegundos;
                tickAtual = tickFinalSegmento;

                if (tickAtual >= tickAlvo) {
                    break;
                }
            }

            return tempoEmSegundos + referenciaChart.offset;
        }
    };
}

// ============================= ESTRUTURA NOTA =============================

/**
 * @brief Representa uma nota no jogo durante a execução
 */
struct Nota {
    double timestampSec = 0.0;
    int tickOriginal = 0;
    int pista = 0;
    int traste = 0;
    float posicaoY = 0.0f;
    bool naTela = false;
    bool acertada = false;
    bool perdida = false;
    Jogador *dono = nullptr;
    sf::Color cor = sf::Color::White;
    bool ehNotaLonga = false;
    double tempoFimSustainSec = 0.0;
    bool sustainAtivo = false;
    bool sustainCompleto = false;
    sf::Time tempoAtePróximaParticulaSustain = sf::Time::Zero;

    /**
     * @brief Construtor da nota
     * @param notaChart Nota do chart original
     * @param timestampSeg Timestamp em segundos
     * @param fimSustainSeg Fim do sustain em segundos
     * @param proprietario Ponteiro para o jogador dono da nota
     */
    Nota(const Chart::NotaChart &notaChart, const double timestampSeg,
         const double fimSustainSeg, Jogador *proprietario)
        : tickOriginal(notaChart.tick), traste(notaChart.traste), timestampSec(timestampSeg),
          tempoFimSustainSec(fimSustainSeg), ehNotaLonga(notaChart.comprimento > 0), dono(proprietario) {

        pista = notaChart.traste;

        // Define cor baseada na pista
        switch (pista) {
            case 0: cor = sf::Color::Green; break;
            case 1: cor = sf::Color::Red; break;
            case 2: cor = sf::Color(255, 255, 0); break; // Amarelo
            case 3: cor = sf::Color::Blue; break;
            case 4: cor = sf::Color(255, 165, 0); break; // Laranja
            default: cor = sf::Color::White; break;
        }
    }

    /**
     * @brief Obtém a posição onde a nota deve ser acertada
     * @return Posição do centro da nota quando acertada
     */
    [[nodiscard]] auto obterPosicaoAcerto() const -> sf::Vector2f {
        if (!dono) return {0.f, 0.f};

        const auto larguraVisualCabeca = static_cast<float>(LARGURA_PISTA - 12);
        const auto xBaseNota = static_cast<float>(dono->offsetAreaJogadorX + pista * LARGURA_PISTA);
        const auto xVisualCabeca = xBaseNota + (LARGURA_PISTA - larguraVisualCabeca) / 2.f;
        const auto xCentro = xVisualCabeca + larguraVisualCabeca / 2.f;

        return {xCentro, posicaoY};
    }

    /**
     * @brief Obtém a posição onde spawnar partículas de sustain
     * @return Posição para partículas de sustain
     */
    [[nodiscard]] auto obterPosicaoParticulaSustain() const -> sf::Vector2f {
        if (!dono) return {0.f, 0.f};

        const auto larguraVisualCabeca = static_cast<float>(LARGURA_PISTA - 12);
        const auto xBaseNota = static_cast<float>(dono->offsetAreaJogadorX + pista * LARGURA_PISTA);
        const auto xVisualCabeca = xBaseNota + (LARGURA_PISTA - larguraVisualCabeca) / 2.f;
        const auto xCentro = xVisualCabeca + larguraVisualCabeca / 2.f;

        return {xCentro, static_cast<float>(Y_ZONA_ACERTO + ALTURA_ZONA_ACERTO / 2.f)};
    }

    /**
     * @brief Obtém os limites da nota para detecção de colisão
     * @param offsetAreaJogadorX Offset X da área do jogador
     * @return Retângulo representando os limites da nota
     */
    [[nodiscard]] auto obterLimites(const int offsetAreaJogadorX) const -> sf::FloatRect {
        const auto x = static_cast<float>(offsetAreaJogadorX + pista * LARGURA_PISTA);
        const auto larguraEfetiva = static_cast<float>(LARGURA_PISTA);
        return {{x, posicaoY - ALTURA_NOTA / 2.f}, {larguraEfetiva, static_cast<float>(ALTURA_NOTA)}};
    }
};

// ============================= CLASSE PRINCIPAL DO JOGO =============================

/**
 * @brief Classe principal que gerencia todo o jogo
 */
class Jogo {
private:
    // Componentes principais
    sf::RenderWindow janela;
    sf::Font fonte;
    std::optional<Chart::DadosChart> dadosChartOpt;
    std::optional<Chart::CalculadoraTempo> calculadoraTempoOpt;

    // Audio
    sf::Music musica;
    sf::SoundBuffer bufferSomAcerto;
    sf::Sound somAcerto;

    // Sistema de partículas
    std::vector<Particula> particulas;
    std::mt19937 motorRandomico;
    std::uniform_real_distribution<float> distribuicaoAnguloParticula;
    std::uniform_real_distribution<float> distribuicaoVelocidadeParticula;
    std::uniform_real_distribution<float> distribuicaoTempoVidaParticula;

    // Shaders e texturas
    sf::Shader shaderNota;
    sf::Shader shaderFundo;
    sf::Texture texturaBranca;
    sf::Texture texturaFundo;
    std::optional<sf::Sprite> spriteBranco;
    std::optional<sf::Sprite> spriteFundo;
    sf::RectangleShape formaPreenchimentoFundo;
    bool shadersDisponiveis = true;

    // Estado do jogo
    bool jogoRodando = false;
    bool jogoIniciado = false;
    bool chartCarregado = false;
    sf::String mensagemStatus;

    // Jogadores
    Jogador jogador1;
    Jogador jogador2;

    // Notas
    std::vector<Nota> todasNotasMusicaMestre;
    std::vector<Nota> notasJ1;
    std::vector<Nota> notasJ2;

    // Timing
    sf::Clock relogioLoopJogo;
    sf::Clock relogioAnimacaoShader;
    sf::Time tempoDesdeUltimaAtualizacao = sf::Time::Zero;
    const sf::Time tempoPorFrame = sf::microseconds(ATUALIZACAO_JOGO_MS * 1000);

public:
    /**
     * @brief Construtor do jogo - inicializa todos os sistemas
     */
    Jogo() : janela(sf::VideoMode({static_cast<unsigned int>(LARGURA_JANELA),
                                   static_cast<unsigned int>(ALTURA_JANELA)}),
                    "SFML Riff Hero", sf::Style::Close | sf::Style::Titlebar),
             jogador1("Jogador 1", 0, {
                         {sf::Keyboard::Key::A, 0}, {sf::Keyboard::Key::S, 1},
                         {sf::Keyboard::Key::D, 2}, {sf::Keyboard::Key::F, 3},
                         {sf::Keyboard::Key::G, 4}
                     }),
             jogador2("Jogador 2", LARGURA_BRASTEADO + LARGURA_PAINEL_CENTRAL, {
                         {sf::Keyboard::Key::J, 0}, {sf::Keyboard::Key::K, 1},
                         {sf::Keyboard::Key::L, 2}, {sf::Keyboard::Key::Semicolon, 3},
                         {sf::Keyboard::Key::Apostrophe, 4}
                     }),
             somAcerto(bufferSomAcerto),
             motorRandomico(std::random_device{}()),
             distribuicaoAnguloParticula(0.f, 2.f * std::numbers::pi_v<float>),
             distribuicaoVelocidadeParticula(VELOCIDADE_PARTICULA_MIN, VELOCIDADE_PARTICULA_MAX),
             distribuicaoTempoVidaParticula(TEMPO_VIDA_PARTICULA_MIN_SEC, TEMPO_VIDA_PARTICULA_MAX_SEC),
             mensagemStatus(utf8ParaSfString("Carregando chart...")) {

        inicializarRecursos();
        carregarDadosChart();
    }

    /**
     * @brief Loop principal do jogo
     */
    void executar() {
        while (janela.isOpen()) {
            const auto dt = relogioLoopJogo.restart();
            tempoDesdeUltimaAtualizacao += dt;

            processarEventos();

            // Loop de atualização com timestep fixo
            while (tempoDesdeUltimaAtualizacao >= tempoPorFrame) {
                tempoDesdeUltimaAtualizacao -= tempoPorFrame;
                if (jogoRodando) {
                    atualizar(tempoPorFrame);
                }
            }

            // Verifica fim de jogo
            if (jogoIniciado && !jogoRodando && !mensagemStatus.toAnsiString().starts_with("Fim de Jogo!")) {
                const sf::String mensagemFim = utf8ParaSfString("Fim de Jogo! J1: " + std::to_string(jogador1.pontuacao) +
                               " J2: " + std::to_string(jogador2.pontuacao) +
                               "\nPressione ESPAÇO para Reiniciar.");
                mensagemStatus = mensagemFim;
                jogoIniciado = false;
            }

            renderizar();
        }
    }

private:
    /**
     * @brief Inicializa recursos básicos (fonte, sons, shaders)
     */
    void inicializarRecursos() {
        // Carrega fonte
        if (!fonte.openFromFile("fonte.ttf")) {
            mensagemStatus = utf8ParaSfString("Erro: Não foi possível carregar a fonte fonte.ttf");
            std::cerr << "Erro: Não foi possível carregar a fonte fonte.ttf" << std::endl;
        }

        // Carrega som de acerto
        if (!bufferSomAcerto.loadFromFile("hit.ogg")) {
            std::cerr << "Erro: Não foi possível carregar hit.ogg" << std::endl;
        }

        // Inicializa shaders
        shadersDisponiveis = sf::Shader::isAvailable();
        if (shadersDisponiveis) {
            if (!shaderNota.loadFromFile("shader_notas.vsh", "shader_notas.fsh")) {
                std::cerr << "Erro ao carregar shader de nota. Usando renderização padrão." << std::endl;
            }
            if (!shaderFundo.loadFromFile("shader_fundo.vsh", "shader_fundo.fsh")) {
                std::cerr << "Erro ao carregar shader de fundo. Usando cor sólida." << std::endl;
            }
        } else {
            std::cerr << "Shaders não estão disponíveis neste sistema." << std::endl;
        }

        // Carrega texturas
        if (!texturaBranca.loadFromFile("branco.png")) {
            std::cerr << "Erro ao carregar textura branco.png." << std::endl;
        }
        if (!texturaFundo.loadFromFile("background.png")) {
            std::cerr << "Erro ao carregar textura background.png." << std::endl;
        }

        // Inicializa sprites
        spriteBranco.emplace(texturaBranca);
        spriteFundo.emplace(texturaFundo);

        // Configura forma de preenchimento do fundo
        formaPreenchimentoFundo.setSize({static_cast<float>(LARGURA_JANELA),
                                        static_cast<float>(ALTURA_JANELA)});
        formaPreenchimentoFundo.setPosition({0.f, 0.f});

        // Configurações da janela
        janela.setVerticalSyncEnabled(true);
    }

    /**
     * @brief Carrega e processa dados do chart
     */
    void carregarDadosChart() {
        mensagemStatus = utf8ParaSfString("Fazendo parsing do arquivo de chart...");

        const auto chartProcessado = Chart::ParserChart::fazerParsingChart(CAMINHO_ARQUIVO_CHART);
        if (!chartProcessado) {
            mensagemStatus = utf8ParaSfString("Erro: Falha no parsing do chart para " + std::string(CAMINHO_ARQUIVO_CHART));
            chartCarregado = false;
            return;
        }

        dadosChartOpt = *chartProcessado;
        calculadoraTempoOpt.emplace(*dadosChartOpt);

        mensagemStatus = utf8ParaSfString("Convertendo notas...");
        todasNotasMusicaMestre.clear();

        if (dadosChartOpt) {
            for (const auto &notaChart : dadosChartOpt->notas) {
                if (notaChart.traste < NUMERO_PISTAS) {
                    const auto tempoNotaSec = calculadoraTempoOpt->ticksParaSegundos(notaChart.tick);
                    auto tempoFimSustainSec = tempoNotaSec;

                    if (notaChart.comprimento > 0) {
                        tempoFimSustainSec = calculadoraTempoOpt->ticksParaSegundos(
                            notaChart.tick + notaChart.comprimento);
                    }

                    // Cria notas para ambos os jogadores
                    todasNotasMusicaMestre.emplace_back(notaChart, tempoNotaSec, tempoFimSustainSec, &jogador1);
                    todasNotasMusicaMestre.emplace_back(notaChart, tempoNotaSec, tempoFimSustainSec, &jogador2);
                }
            }
        }

        // Ordena notas por timestamp
        std::ranges::sort(todasNotasMusicaMestre, [](const auto &a, const auto &b) {
            return a.timestampSec < b.timestampSec;
        });

        carregarAudio();
    }

    /**
     * @brief Carrega arquivo de áudio
     */
    void carregarAudio() {
        mensagemStatus = utf8ParaSfString("Carregando áudio...");

        std::string nomeArquivoAudio = dadosChartOpt->streamMusica.isEmpty() ?
                                      "song.ogg" : sfStringParaUtf8(dadosChartOpt->streamMusica);

        if (!musica.openFromFile(nomeArquivoAudio)) {
            // Tenta extensões alternativas
            const auto posicaoPonto = nomeArquivoAudio.rfind('.');
            const std::string nomeBase = (posicaoPonto == std::string::npos) ?
                                        nomeArquivoAudio : nomeArquivoAudio.substr(0, posicaoPonto);

            constexpr std::array<std::string_view, 4> extensoes = {".ogg", ".wav", ".flac", ".mp3"};

            const bool encontrado = std::ranges::any_of(extensoes, [&](const std::string_view ext) {
                return musica.openFromFile(nomeBase + std::string(ext));
            });

            if (!encontrado) {
                mensagemStatus = utf8ParaSfString("Erro: Arquivo de áudio não encontrado. Tentou: " +
                               nomeArquivoAudio + " e variantes.");
                std::cerr << "Erro: Arquivo de áudio não encontrado. Tentou: " <<
                          nomeArquivoAudio << " e variantes." << std::endl;
                chartCarregado = false;
                return;
            }
        }

        chartCarregado = true;

        if (fonte.getInfo().family.empty()) {
            mensagemStatus = utf8ParaSfString("Erro: Fonte não carregada. Texto não será exibido.");
        } else {
            const std::string textoStatus = "Pressione ESPAÇO para Iniciar!";
            mensagemStatus = utf8ParaSfString(textoStatus);
        }

        std::cout << "Chart carregado. Notas: " << todasNotasMusicaMestre.size() / 2
                  << ". Música: " << sfStringParaUtf8(dadosChartOpt->nome) << " por "
                  << sfStringParaUtf8(dadosChartOpt->artista) << std::endl;
    }

    /**
     * @brief Inicia uma nova partida
     */
    void iniciarJogo() {
        if (!chartCarregado || jogoIniciado) {
            if (!chartCarregado) {
                mensagemStatus = utf8ParaSfString("Chart não carregado ou ocorreu um erro.");
            }
            return;
        }

        // Reseta estado dos jogadores
        jogador1.pontuacao = 0;
        jogador1.teclasPresionadas.clear();
        for (auto &entrada : jogador1.pistaPermiteAcertoNotaCurta) entrada.second = true;

        jogador2.pontuacao = 0;
        jogador2.teclasPresionadas.clear();
        for (auto &entrada : jogador2.pistaPermiteAcertoNotaCurta) entrada.second = true;

        // Limpa estados
        notasJ1.clear();
        notasJ2.clear();
        particulas.clear();

        // Cria instâncias das notas para o jogo
        for (const auto &notaModelo : todasNotasMusicaMestre) {
            Nota instanciaNota = notaModelo;
            instanciaNota.tempoAtePróximaParticulaSustain = sf::Time::Zero;

            if (instanciaNota.dono == &jogador1) {
                notasJ1.push_back(std::move(instanciaNota));
            } else if (instanciaNota.dono == &jogador2) {
                notasJ2.push_back(std::move(instanciaNota));
            }
        }

        // Ordena notas por tempo
        const auto ordenarPorTempo = [](const auto &a, const auto &b) {
            return a.timestampSec < b.timestampSec;
        };
        std::ranges::sort(notasJ1, ordenarPorTempo);
        std::ranges::sort(notasJ2, ordenarPorTempo);

        // Inicia jogo
        jogoIniciado = true;
        jogoRodando = true;
        mensagemStatus = utf8ParaSfString("Tocando...");

        musica.stop();
        musica.setPlayingOffset(sf::Time::Zero);
        musica.play();

        relogioLoopJogo.restart();
        tempoDesdeUltimaAtualizacao = sf::Time::Zero;
    }

    /**
     * @brief Quebra texto em linhas que cabem na largura especificada
     * @param texto Texto a ser quebrado (sf::String)
     * @param fonte Fonte a ser usada
     * @param tamanhoFonte Tamanho da fonte
     * @param larguraMaxima Largura máxima permitida
     * @return Vector de sf::String com as linhas
     */
    [[nodiscard]] std::vector<sf::String> quebrarTexto(const sf::String& texto,
                                                       const sf::Font& fonte,
                                                       const unsigned int tamanhoFonte,
                                                       const float larguraMaxima) const {
        std::vector<sf::String> linhas;
        if (texto.isEmpty()) return linhas;

        sf::Text textoTeste(fonte, "", tamanhoFonte);

        // Converte sf::String para std::string para processamento de palavras
        std::string textoUtf8 = sfStringParaUtf8(texto);
        std::istringstream palavras(textoUtf8);
        std::string palavra;
        std::string linhaAtual;

        while (palavras >> palavra) {
            const std::string linhaTestada = linhaAtual.empty() ? palavra : linhaAtual + " " + palavra;
            textoTeste.setString(utf8ParaSfString(linhaTestada));

            if (textoTeste.getLocalBounds().size.x <= larguraMaxima) {
                linhaAtual = linhaTestada;
            } else {
                if (!linhaAtual.empty()) {
                    linhas.push_back(utf8ParaSfString(linhaAtual));
                    linhaAtual = palavra;
                } else {
                    // Palavra única muito longa
                    linhas.push_back(utf8ParaSfString(palavra));
                }
            }
        }

        if (!linhaAtual.empty()) {
            linhas.push_back(utf8ParaSfString(linhaAtual));
        }

        return linhas;
    }

    /**
     * @brief Desenha texto quebrado e retorna a altura usada
     * @param texto Texto a ser desenhado (sf::String)
     * @param cor Cor do texto
     * @param tamanhoFonte Tamanho da fonte
     * @param x Posição X
     * @param y Posição Y
     * @param larguraMaxima Largura máxima
     * @param centralizado Se deve centralizar o texto
     * @return Altura total usada pelo texto
     */
    float desenharTextoQuebrado(const sf::String& texto, const sf::Color cor,
                               const unsigned int tamanhoFonte, const float x, const float y,
                               const float larguraMaxima, const bool centralizado = true) {
        if (fonte.getInfo().family.empty() || texto.isEmpty()) return 0.f;

        const auto linhas = quebrarTexto(texto, fonte, tamanhoFonte, larguraMaxima);
        float alturaTotal = 0.f;
        float yAtual = y;

        for (const auto& linha : linhas) {
            sf::Text textoLinha(fonte, linha, tamanhoFonte);
            textoLinha.setFillColor(cor);
            const auto limites = textoLinha.getLocalBounds();

            if (centralizado) {
                textoLinha.setOrigin({std::round(limites.position.x + limites.size.x / 2.f),
                                     std::round(limites.position.y)});
                textoLinha.setPosition({std::round(x), std::round(yAtual)});
            } else {
                textoLinha.setOrigin({std::round(limites.position.x), std::round(limites.position.y)});
                textoLinha.setPosition({std::round(x - larguraMaxima / 2.f), std::round(yAtual)});
            }

            janela.draw(textoLinha);
            yAtual += limites.size.y + 2.f;
            alturaTotal += limites.size.y + 2.f;
        }

        return alturaTotal;
    }

    /**
     * @brief Processa eventos de entrada
     */
    void processarEventos() {
        std::optional<sf::Event> eventoOpt;
        while ((eventoOpt = janela.pollEvent())) {
            if (eventoOpt->is<sf::Event::Closed>()) {
                janela.close();
            } else if (const auto *teclaPress = eventoOpt->getIf<sf::Event::KeyPressed>()) {
                processarTeclaPress(teclaPress->code);
            } else if (const auto *teclaRelease = eventoOpt->getIf<sf::Event::KeyReleased>()) {
                processarTeclaRelease(teclaRelease->code);
            }
        }
    }

    /**
     * @brief Atualiza lógica do jogo
     * @param dt Delta time
     */
    void atualizar(const sf::Time dt) {
        if (!jogoRodando) return;

        const auto dtSec = dt.asSeconds();
        const auto tempoAudioBruto = musica.getPlayingOffset();
        const auto tempoAtualMusicaSec = tempoAudioBruto.asSeconds() + OFFSET_LATENCIA_AUDIO_SEC;

        atualizarLogicaJogador(notasJ1, tempoAtualMusicaSec, dtSec);
        atualizarLogicaJogador(notasJ2, tempoAtualMusicaSec, dtSec);
        atualizarSustainParaJogador(jogador1, notasJ1, tempoAtualMusicaSec, dt);
        atualizarSustainParaJogador(jogador2, notasJ2, tempoAtualMusicaSec, dt);
        atualizarParticulas(dt);

        // Verifica fim da música
        if (musica.getStatus() != sf::SoundSource::Status::Playing && jogoIniciado) {
            const auto temNotasAtivas = [](const std::vector<Nota> &notas) {
                return std::ranges::any_of(notas, [](const Nota &nota) {
                    return nota.naTela && !nota.acertada && !nota.perdida;
                });
            };

            if (!temNotasAtivas(notasJ1) && !temNotasAtivas(notasJ2)) {
                jogoRodando = false;
            }
        }
    }

    /**
     * @brief Atualiza lógica das notas de um jogador
     * @param notasJogador Notas do jogador
     * @param tempoMusicaSec Tempo atual da música em segundos
     * @param dtSec Delta time em segundos (não usado)
     */
    void atualizarLogicaJogador(std::vector<Nota> &notasJogador, const double tempoMusicaSec,
                               const float /*dtSec_naoUsado*/) {
        for (auto &nota : notasJogador) {
            // Sempre atualiza posição para que notas continuem caindo naturalmente
            const auto tempoAteAcerto = nota.timestampSec - tempoMusicaSec;
            const auto yAlvo = static_cast<float>(Y_ZONA_ACERTO - (tempoAteAcerto * VELOCIDADE_QUEDA_NOTA_PPS));
            nota.posicaoY = yAlvo;

            // Pula lógica para notas perdidas, mas ainda atualiza visibilidade
            if (nota.perdida) {
                atualizarVisibilidadeNota(nota);
                continue;
            }

            atualizarVisibilidadeNota(nota);

            if (nota.naTela && !nota.acertada) {
                verificarNotaPerdida(nota, tempoMusicaSec);
            }
        }
    }

    /**
     * @brief Atualiza visibilidade de uma nota
     * @param nota Nota a ter visibilidade atualizada
     */
    void atualizarVisibilidadeNota(Nota &nota) const {
        constexpr float raioCapeca = ALTURA_NOTA / 2.f;
        const bool cabecaPotencialmenteVisivel = (nota.posicaoY + raioCapeca > 0) &&
                                                (nota.posicaoY - raioCapeca < ALTURA_JANELA);
        bool caldaPotencialmenteVisivel = false;

        if (nota.ehNotaLonga) {
            const auto pixelsSustain = static_cast<float>(
                (nota.tempoFimSustainSec - nota.timestampSec) * VELOCIDADE_QUEDA_NOTA_PPS);
            const auto yTopoRealCalda = nota.posicaoY - std::max(0.f, pixelsSustain);
            const auto yFundoRealCalda = nota.posicaoY;
            caldaPotencialmenteVisivel = (yFundoRealCalda > 0) && (yTopoRealCalda < ALTURA_JANELA);
        }

        if ((cabecaPotencialmenteVisivel || caldaPotencialmenteVisivel) && !nota.naTela) {
            nota.naTela = true;
        }

        if (nota.naTela) {
            bool aindaRealmenteNaTela = true;

            if (nota.posicaoY + raioCapeca < 0) {
                aindaRealmenteNaTela = false;
                if (!nota.acertada && !nota.perdida) nota.perdida = true;
            } else {
                const float yVisualMaisAlto = nota.ehNotaLonga
                    ? (nota.posicaoY - static_cast<float>((nota.tempoFimSustainSec - nota.timestampSec) * VELOCIDADE_QUEDA_NOTA_PPS))
                    : (nota.posicaoY - raioCapeca);

                if (yVisualMaisAlto > ALTURA_JANELA) {
                    aindaRealmenteNaTela = false;
                    if (!nota.acertada && !nota.perdida) nota.perdida = true;
                    if (nota.ehNotaLonga && nota.acertada && !nota.sustainCompleto) nota.perdida = true;
                }
            }

            nota.naTela = aindaRealmenteNaTela;
        }
    }

    /**
     * @brief Verifica se uma nota deve ser marcada como perdida
     * @param nota Nota a verificar
     * @param tempoMusicaSec Tempo atual da música
     */
    void verificarNotaPerdida(Nota &nota, const double tempoMusicaSec) const {
        constexpr float raioCapeca = ALTURA_NOTA / 2.f;

        if (!nota.ehNotaLonga && nota.posicaoY > Y_ZONA_ACERTO + ALTURA_ZONA_ACERTO + raioCapeca) {
            nota.perdida = true;
        } else if (nota.ehNotaLonga && tempoMusicaSec > nota.tempoFimSustainSec +
                  (static_cast<double>(TOLERANCIA_ACERTO_MS) / 1000.0)) {
            nota.perdida = true;
        }
    }

    /**
     * @brief Atualiza sistema de sustain para um jogador
     * @param jogador Jogador
     * @param notas Notas do jogador
     * @param tempoMusicaSec Tempo atual da música
     * @param dt Delta time
     */
    void atualizarSustainParaJogador(Jogador &jogador, std::vector<Nota> &notas,
                                    const double tempoMusicaSec, const sf::Time dt) {
        for (auto &nota : notas) {
            if (!nota.ehNotaLonga || nota.sustainCompleto || nota.perdida ||
                !nota.naTela || !nota.acertada) {
                continue;
            }

            const bool teclaPresionadaParaPista = std::ranges::any_of(
                jogador.mapeamentoTeclaPista, [&](const auto &par) {
                    return par.second == nota.pista && jogador.teclasPresionadas.contains(par.first);
                });

            const bool dentroPeríodoSustain = tempoMusicaSec >= nota.timestampSec &&
                                             tempoMusicaSec <= nota.tempoFimSustainSec;

            if (teclaPresionadaParaPista && dentroPeríodoSustain) {
                nota.sustainAtivo = true;
                jogador.adicionarPontuacao(1);

                nota.tempoAtePróximaParticulaSustain -= dt;
                if (nota.tempoAtePróximaParticulaSustain <= sf::Time::Zero) {
                    spawnarParticulasSustain(nota, jogador);
                    nota.tempoAtePróximaParticulaSustain = INTERVALO_SPAWN_PARTICULA_SUSTAIN;
                }
            } else {
                nota.sustainAtivo = false;
            }

            if (tempoMusicaSec > nota.tempoFimSustainSec && nota.sustainAtivo) {
                nota.sustainCompleto = true;
                nota.sustainAtivo = false;
                jogador.adicionarPontuacao(20);
            }
        }
    }

    /**
     * @brief Spawna partículas para efeito de sustain
     * @param nota Nota que está em sustain
     * @param jogador Jogador (não usado atualmente)
     */
    void spawnarParticulasSustain(const Nota &nota, const Jogador & /*jogador*/) {
        const auto posicaoParticula = nota.obterPosicaoParticulaSustain();

        for (int i = 0; i < PARTICULAS_SUSTAIN; ++i) {
            Particula p;
            p.forma.setSize({TAMANHO_PARTICULA, TAMANHO_PARTICULA});
            p.forma.setFillColor(sf::Color(nota.cor.r, nota.cor.g, nota.cor.b, 150));
            p.forma.setOrigin({TAMANHO_PARTICULA / 2.f, TAMANHO_PARTICULA / 2.f});
            p.posicao = posicaoParticula;

            const auto angulo = distribuicaoAnguloParticula(motorRandomico) * 0.3f -
                               (std::numbers::pi_v<float> / 2.f) * 0.3f -
                               (std::numbers::pi_v<float> / 2.f) * 0.85f;
            auto velocidade = distribuicaoVelocidadeParticula(motorRandomico) * 0.5f;
            if (velocidade > VELOCIDADE_PARTICULA_SUSTAIN_MAX) {
                velocidade = VELOCIDADE_PARTICULA_SUSTAIN_MAX;
            }

            p.velocidade = {(std::cos(angulo) * velocidade) * 5.f, (std::sin(angulo) * velocidade) * 5.f};
            p.tempoVida = sf::seconds(distribuicaoTempoVidaParticula(motorRandomico) * 0.7f);
            particulas.push_back(p);
        }
    }

    /**
     * @brief Spawna partículas para efeito de acerto
     * @param posicao Posição onde spawnar
     * @param cor Cor das partículas
     */
    void spawnarParticulas(const sf::Vector2f posicao, const sf::Color cor) {
        for (int i = 0; i < PARTICULAS_POR_ACERTO; ++i) {
            Particula p;
            p.forma.setSize({TAMANHO_PARTICULA, TAMANHO_PARTICULA});
            p.forma.setFillColor(cor);
            p.forma.setOrigin({TAMANHO_PARTICULA / 2.f, TAMANHO_PARTICULA / 2.f});
            p.posicao = posicao;

            const auto angulo = distribuicaoAnguloParticula(motorRandomico);
            const auto velocidade = distribuicaoVelocidadeParticula(motorRandomico);
            p.velocidade = {std::cos(angulo) * velocidade, std::sin(angulo) * velocidade};
            p.tempoVida = sf::seconds(distribuicaoTempoVidaParticula(motorRandomico));
            particulas.push_back(p);
        }
    }

    /**
     * @brief Atualiza sistema de partículas
     * @param dt Delta time
     */
    void atualizarParticulas(const sf::Time dt) {
        std::erase_if(particulas, [&](Particula &p) {
            p.tempoVida -= dt;
            if (p.tempoVida <= sf::Time::Zero) {
                return true;
            }

            p.posicao += p.velocidade * dt.asSeconds();
            p.forma.setPosition(p.posicao);

            const float proporcaoTempoVida = p.tempoVida.asSeconds() / TEMPO_VIDA_PARTICULA_MAX_SEC;
            sf::Color cor = p.forma.getFillColor();
            cor.a = static_cast<std::uint8_t>(std::max(0.f, 255.f * proporcaoTempoVida));
            p.forma.setFillColor(cor);

            return false;
        });
    }

    /**
     * @brief Processa tecla pressionada
     * @param tecla Tecla que foi pressionada
     */
    void processarTeclaPress(const sf::Keyboard::Key tecla) {
        if (tecla == sf::Keyboard::Key::Space && !jogoIniciado && chartCarregado) {
            iniciarJogo();
            return;
        }

        if (!jogoRodando) return;

        const auto processarTeclaPressJogador = [&](Jogador &jogador, std::vector<Nota> &notasJogador) {
            const auto it = jogador.mapeamentoTeclaPista.find(tecla);
            if (it != jogador.mapeamentoTeclaPista.end()) {
                jogador.teclasPresionadas.insert(tecla);
                const int pista = it->second;

                if (jogador.pistaPermiteAcertoNotaCurta[pista]) {
                    const auto tempoAudioBruto = musica.getPlayingOffset();
                    const auto tempoAtualMusicaSec = tempoAudioBruto.asSeconds() + OFFSET_LATENCIA_AUDIO_SEC;
                    const bool notaCurtaFoiAcertada = verificarAcertoNota(jogador, notasJogador,
                                                                        pista, tempoAtualMusicaSec);
                    if (notaCurtaFoiAcertada) {
                        jogador.pistaPermiteAcertoNotaCurta[pista] = false;
                    }
                }
            }
        };

        processarTeclaPressJogador(jogador1, notasJ1);
        processarTeclaPressJogador(jogador2, notasJ2);
    }

    /**
     * @brief Processa tecla liberada
     * @param tecla Tecla que foi liberada
     */
    void processarTeclaRelease(const sf::Keyboard::Key tecla) {
        if (!jogoRodando) return;

        const auto processarTeclaReleaseJogador = [&](Jogador &jogador) {
            const auto it = jogador.mapeamentoTeclaPista.find(tecla);
            if (it != jogador.mapeamentoTeclaPista.end()) {
                jogador.teclasPresionadas.erase(tecla);
                jogador.pistaPermiteAcertoNotaCurta[it->second] = true;
            }
        };

        processarTeclaReleaseJogador(jogador1);
        processarTeclaReleaseJogador(jogador2);
    }

    /**
     * @brief Verifica se uma nota foi acertada
     * @param jogador Jogador que tentou acertar
     * @param notas Notas do jogador
     * @param pistaAlvo Pista da tecla pressionada
     * @param tempoMusicaSec Tempo atual da música
     * @return True se alguma nota foi acertada
     */
    bool verificarAcertoNota(Jogador &jogador, std::vector<Nota> &notas,
                           const int pistaAlvo, const double tempoMusicaSec) {
        for (auto &nota : notas) {
            if (nota.pista == pistaAlvo && !nota.acertada && !nota.perdida && nota.naTela &&
                nota.posicaoY >= (Y_ZONA_ACERTO - ALTURA_NOTA) &&
                nota.posicaoY <= (Y_ZONA_ACERTO + ALTURA_ZONA_ACERTO + ALTURA_NOTA) &&
                (std::abs(nota.timestampSec - tempoMusicaSec) * 1000.0) <= TOLERANCIA_ACERTO_MS) {

                nota.acertada = true;

                // Toca som de acerto
                if (bufferSomAcerto.getSampleCount() > 0) {
                    somAcerto.play();
                }

                // Spawna partículas
                spawnarParticulas(nota.obterPosicaoAcerto(), nota.cor);

                // Adiciona pontuação
                if (nota.ehNotaLonga) {
                    jogador.adicionarPontuacao(5);
                } else {
                    jogador.adicionarPontuacao(10);
                }
            }
        }

        return false;  // Mantido como estava no código original
    }

    /**
     * @brief Renderiza todos os elementos do jogo
     */
    void renderizar() {
        janela.clear(sf::Color::Black);

        // Desenha fundo com shader se disponível
        if (shadersDisponiveis && shaderFundo.getNativeHandle() != 0) {
            shaderFundo.setUniform(UNIFORM_RESOLUCAO, sf::Glsl::Vec2{LARGURA_JANELA, ALTURA_JANELA});
            shaderFundo.setUniform(UNIFORM_TEMPO, relogioAnimacaoShader.getElapsedTime().asSeconds());
            janela.draw(formaPreenchimentoFundo, &shaderFundo);
        } else {
            janela.draw(formaPreenchimentoFundo);
        }

        if (chartCarregado && dadosChartOpt) {
            desenharAreaJogador(jogador1, notasJ1);
            desenharAreaJogador(jogador2, notasJ2);
        }

        desenharPainelCentral();
        desenharParticulas();
        janela.display();
    }

    /**
     * @brief Desenha área de um jogador (brasteado + notas)
     * @param jogador Jogador
     * @param notas Notas do jogador
     */
    void desenharAreaJogador(const Jogador &jogador, const std::vector<Nota> &notas) {
        desenharBrasteado(jogador);
        desenharNotasJogo(notas, jogador);
    }

    /**
     * @brief Desenha o painel central unificado
     */
    void desenharPainelCentral() {
        if (fonte.getInfo().family.empty()) return;

        // Calcula posição do painel central (sem margens superior e inferior)
        constexpr auto xPainel = static_cast<float>(LARGURA_BRASTEADO);
        constexpr auto yPainel = 0.f;  // Remove margem superior
        constexpr auto larguraPainel = static_cast<float>(LARGURA_PAINEL_CENTRAL);
        constexpr auto alturaPainel = static_cast<float>(ALTURA_JANELA);  // Remove margem inferior

        // Desenha fundo do painel central com borda cinza
        sf::RectangleShape fundoPainel({larguraPainel, alturaPainel});
        fundoPainel.setPosition({xPainel, yPainel});
        fundoPainel.setFillColor(sf::Color(20, 20, 30, 200));
        fundoPainel.setOutlineColor(sf::Color(128, 128, 128));  // Borda cinza
        fundoPainel.setOutlineThickness(2.f);
        janela.draw(fundoPainel);

        auto yAtual = std::round(yPainel + 20.f);
        const auto xCentroTexto = std::round(xPainel + larguraPainel / 2.f);

        // Desenha informações da música se chart estiver carregado
        if (chartCarregado && dadosChartOpt) {
            const float larguraMaxTexto = larguraPainel - 20.f; // Margem de 10px de cada lado

            // Título da música (2x maior)
            yAtual += desenharTextoQuebrado(dadosChartOpt->nome, sf::Color::White, 28,
                                          xCentroTexto, yAtual, larguraMaxTexto);
            yAtual += 8.f;

            // Artista (maior também)
            const sf::String textoArtista = utf8ParaSfString("por ") + dadosChartOpt->artista;
            yAtual += desenharTextoQuebrado(textoArtista, sf::Color(160, 160, 160), 20,
                                          xCentroTexto, yAtual, larguraMaxTexto);
            yAtual += 32.f;

            // Álbum (se disponível) - fonte maior, cinza
            if (!dadosChartOpt->album.isEmpty()) {
                yAtual += desenharTextoQuebrado(dadosChartOpt->album, sf::Color(160, 160, 160), 16,
                                              xCentroTexto, yAtual, larguraMaxTexto);
                yAtual += 5.f;
            }

            // Ano e Gênero (se disponíveis) - fonte maior, cinza
            sf::String anoGenero;
            if (!dadosChartOpt->ano.isEmpty()) {
                anoGenero += dadosChartOpt->ano;
            }
            if (!dadosChartOpt->genero.isEmpty()) {
                if (!anoGenero.isEmpty()) anoGenero += utf8ParaSfString(" • ");
                anoGenero += dadosChartOpt->genero;
            }
            if (!anoGenero.isEmpty()) {
                yAtual += desenharTextoQuebrado(anoGenero, sf::Color(160, 160, 160), 16,
                                              xCentroTexto, yAtual, larguraMaxTexto);
                yAtual += 5.f;
            }

            // Criador do chart (se disponível) - fonte maior, cinza
            if (!dadosChartOpt->criadorChart.isEmpty()) {
                const sf::String textoCreator = utf8ParaSfString("notas por ") + dadosChartOpt->criadorChart;
                yAtual += desenharTextoQuebrado(textoCreator, sf::Color(160, 160, 160), 18,
                                              xCentroTexto, yAtual, larguraMaxTexto);
                yAtual += 15.f;
            } else {
                yAtual += 15.f;
            }
        }

        // Função auxiliar para formatar pontuação com vírgulas
        auto formatarPontuacao = [](int pontuacao) -> std::string {
            std::string str = std::to_string(pontuacao);
            std::string resultado;
            int contador = 0;

            for (int i = str.length() - 1; i >= 0; --i) {
                if (contador > 0 && contador % 3 == 0) {
                    resultado = "," + resultado;
                }
                resultado = str[i] + resultado;
                contador++;
            }

            return resultado;
        };

        // Desenha pontuações formatadas com cores diferentes
        const sf::String textoP1 = jogador1.nome + utf8ParaSfString("\n" + formatarPontuacao(jogador1.pontuacao));
        sf::Text pontuacaoP1(fonte, textoP1, 22);
        pontuacaoP1.setFillColor(sf::Color(255, 100, 100));  // Vermelho claro para P1
        const auto limitesP1 = pontuacaoP1.getLocalBounds();
        pontuacaoP1.setOrigin({limitesP1.position.x + limitesP1.size.x / 2.f, limitesP1.position.y});
        pontuacaoP1.setPosition({xCentroTexto, yAtual});
        janela.draw(pontuacaoP1);
        yAtual += limitesP1.size.y + 25.f;

        const sf::String textoP2 = jogador2.nome + utf8ParaSfString("\n" + formatarPontuacao(jogador2.pontuacao));
        sf::Text pontuacaoP2(fonte, textoP2, 22);
        pontuacaoP2.setFillColor(sf::Color(100, 150, 255));  // Azul claro para P2
        const auto limitesP2 = pontuacaoP2.getLocalBounds();
        pontuacaoP2.setOrigin({limitesP2.position.x + limitesP2.size.x / 2.f, limitesP2.position.y});
        pontuacaoP2.setPosition({xCentroTexto, yAtual});
        janela.draw(pontuacaoP2);
        yAtual += limitesP2.size.y + 35.f;

        // Desenha tempo da música se estiver tocando
        if ((musica.getStatus() == sf::SoundSource::Status::Playing || jogoIniciado) && chartCarregado) {
            const auto tempoAudioBruto = musica.getPlayingOffset();
            const auto tempoEfetivoMusicaSec = tempoAudioBruto.asSeconds() + OFFSET_LATENCIA_AUDIO_SEC;
            std::stringstream streamTempo;
            streamTempo << "Tempo: " << std::fixed << std::setprecision(1) << tempoEfetivoMusicaSec << "s";

            sf::Text textoTempo(fonte, utf8ParaSfString(streamTempo.str()), 18);
            textoTempo.setFillColor(sf::Color::White);
            const auto limitesTempo = textoTempo.getLocalBounds();
            textoTempo.setOrigin({limitesTempo.position.x + limitesTempo.size.x / 2.f, limitesTempo.position.y});
            textoTempo.setPosition({xCentroTexto, yAtual});
            janela.draw(textoTempo);
            yAtual += limitesTempo.size.y + 35.f;
        }

        // Desenha mensagens de status (mantido como estava)
        if (!mensagemStatus.isEmpty()) {
            sf::Color corMensagem = sf::Color::White;
            const std::string mensagemUtf8 = sfStringParaUtf8(mensagemStatus);
            if (mensagemUtf8.starts_with("Erro")) {
                corMensagem = sf::Color::Red;
            } else if (mensagemUtf8.starts_with("Fim de Jogo!")) {
                corMensagem = sf::Color::Yellow;
            } else if (!jogoIniciado && chartCarregado) {
                corMensagem = sf::Color::Green;
            }

            const float larguraMaxTexto = larguraPainel - 20.f;

            // Divide mensagem de status em linhas e desenha cada uma com quebra
            std::string mensagemUtf8Str = sfStringParaUtf8(mensagemStatus);
            std::stringstream ss(mensagemUtf8Str);
            std::string linha;
            while (std::getline(ss, linha, '\n')) {
                yAtual += desenharTextoQuebrado(utf8ParaSfString(linha), corMensagem, 18,
                                              xCentroTexto, yAtual, larguraMaxTexto);
                yAtual += 3.f;
            }
            yAtual += 10.f;
        }

        // Desenha informações de controles no final se não estiver jogando
        if (!jogoRodando) {
            const float yControles = alturaPainel - 120.f;

            sf::Text tituloControles(fonte, utf8ParaSfString("Controles:"), 20);
            tituloControles.setFillColor(sf::Color(200, 200, 200));
            const auto limitesTituloCtrl = tituloControles.getLocalBounds();
            tituloControles.setOrigin({limitesTituloCtrl.position.x + limitesTituloCtrl.size.x / 2.f,
                                     limitesTituloCtrl.position.y});
            tituloControles.setPosition({xCentroTexto, yControles});
            janela.draw(tituloControles);

            float yControlesAtual = yControles + limitesTituloCtrl.size.y + 12.f;

            sf::Text controlesP1(fonte, utf8ParaSfString("J1: A S D F G"), 18);
            controlesP1.setFillColor(sf::Color(180, 180, 180));
            const auto limitesCtrlP1 = controlesP1.getLocalBounds();
            controlesP1.setOrigin({limitesCtrlP1.position.x + limitesCtrlP1.size.x / 2.f,
                                 limitesCtrlP1.position.y});
            controlesP1.setPosition({xCentroTexto, yControlesAtual});
            janela.draw(controlesP1);
            yControlesAtual += limitesCtrlP1.size.y + 8.f;

            sf::Text controlesP2(fonte, utf8ParaSfString("J2: J K L ; '"), 18);
            controlesP2.setFillColor(sf::Color(180, 180, 180));
            const auto limitesCtrlP2 = controlesP2.getLocalBounds();
            controlesP2.setOrigin({limitesCtrlP2.position.x + limitesCtrlP2.size.x / 2.f,
                                 limitesCtrlP2.position.y});
            controlesP2.setPosition({xCentroTexto, yControlesAtual});
            janela.draw(controlesP2);
        }
    }
    /**
     * @brief Desenha o brasteado (pistas e zona de acerto)
     * @param jogador Jogador
     */
    void desenharBrasteado(const Jogador &jogador) {
        const auto xOffset = static_cast<float>(jogador.offsetAreaJogadorX);

        // Desenha linhas divisórias das pistas
        for (int i = 1; i < NUMERO_PISTAS; ++i) {
            sf::RectangleShape linhaPista({1.f, static_cast<float>(ALTURA_JANELA)});
            linhaPista.setPosition({xOffset + i * LARGURA_PISTA, 0.f});
            linhaPista.setFillColor(sf::Color(100, 100, 100));
            janela.draw(linhaPista);
        }

        // Desenha zona de acerto
        sf::RectangleShape preenchimentoZonaAcerto({static_cast<float>(LARGURA_BRASTEADO),
                                                   static_cast<float>(ALTURA_ZONA_ACERTO)});
        preenchimentoZonaAcerto.setPosition({xOffset, static_cast<float>(Y_ZONA_ACERTO)});
        preenchimentoZonaAcerto.setFillColor(sf::Color(200, 200, 200, 100));
        preenchimentoZonaAcerto.setOutlineColor(sf::Color::White);
        preenchimentoZonaAcerto.setOutlineThickness(1.f);
        janela.draw(preenchimentoZonaAcerto);

        // Desenha feedback visual para teclas pressionadas
        for (const auto &[tecla, pista] : jogador.mapeamentoTeclaPista) {
            if (jogador.teclasPresionadas.contains(tecla)) {
                sf::RectangleShape preenchimentoFeedbackTecla(
                    {static_cast<float>(LARGURA_PISTA), static_cast<float>(ALTURA_ZONA_ACERTO)});
                preenchimentoFeedbackTecla.setPosition({xOffset + pista * LARGURA_PISTA,
                                                       static_cast<float>(Y_ZONA_ACERTO)});
                preenchimentoFeedbackTecla.setFillColor(sf::Color(255, 255, 255, 80));
                janela.draw(preenchimentoFeedbackTecla);
            }
        }
    }

    /**
     * @brief Desenha todas as partículas
     */
    void desenharParticulas() {
        for (const auto &particula : particulas) {
            janela.draw(particula.forma);
        }
    }

    /**
     * @brief Desenha notas do jogo para um jogador
     * @param notas Notas a desenhar
     * @param jogador Jogador dono das notas
     */
    void desenharNotasJogo(const std::vector<Nota> &notas, const Jogador &jogador) {
        sf::RectangleShape formaRetangulo;
        sf::Text marcacaoCompleto(fonte, utf8ParaSfString("✓"), 15);

        auto estadosRenderizacaoNota = sf::RenderStates::Default;
        if (shadersDisponiveis && shaderNota.getNativeHandle() != 0) {
            shaderNota.setUniform(UNIFORM_TEMPO, relogioAnimacaoShader.getElapsedTime().asSeconds());
            shaderNota.setUniform(UNIFORM_TEXTURA, sf::Shader::CurrentTexture);
            estadosRenderizacaoNota.shader = &shaderNota;
        }

        for (const auto &nota : notas) {
            if (!nota.naTela) continue;

            const auto xBaseNota = static_cast<float>(jogador.offsetAreaJogadorX + nota.pista * LARGURA_PISTA);
            const auto yCentroCapeca = nota.posicaoY;

            auto larguraVisualCapeca = static_cast<float>(LARGURA_PISTA - 12);
            if (larguraVisualCapeca < ALTURA_NOTA) larguraVisualCapeca = static_cast<float>(ALTURA_NOTA);

            constexpr auto alturaCapeca = static_cast<float>(ALTURA_NOTA);
            constexpr auto raio = alturaCapeca / 2.f;

            const auto xVisualCapeca = xBaseNota + (LARGURA_PISTA - larguraVisualCapeca) / 2.f;
            const auto yTopoVisualCapeca = yCentroCapeca - raio;

            // Desenha cauda da nota longa se aplicável
            if (nota.ehNotaLonga && (!nota.perdida || nota.acertada)) {
                const auto comprimentoSustainSec = nota.tempoFimSustainSec - nota.timestampSec;
                const auto pixelsSustain = static_cast<float>(comprimentoSustainSec * VELOCIDADE_QUEDA_NOTA_PPS);

                if (pixelsSustain > 0) {
                    const auto corCalda = nota.sustainAtivo
                                             ? sf::Color(nota.cor.r, nota.cor.g, nota.cor.b, 255)
                                             : sf::Color(nota.cor.r, nota.cor.g, nota.cor.b, 100);

                    // Calcula largura da cauda independentemente da altura da nota
                    const auto larguraMaxCalda = larguraVisualCapeca * 0.8f; // 80% da largura da cabeça
                    auto larguraCalda = std::min(larguraMaxCalda, static_cast<float>(LARGURA_PISTA) * 0.7f);
                    if (larguraCalda < 4.f) larguraCalda = 4.f;

                    const auto xCalda = xVisualCapeca + (larguraVisualCapeca - larguraCalda) / 2.f;

                    formaRetangulo.setSize({larguraCalda, pixelsSustain});
                    formaRetangulo.setPosition({xCalda, yCentroCapeca - pixelsSustain});
                    formaRetangulo.setFillColor(corCalda);

                    if (spriteBranco) formaRetangulo.setTexture(&spriteBranco->getTexture());
                    else formaRetangulo.setTexture(nullptr);

                    if (estadosRenderizacaoNota.shader) {
                        shaderNota.setUniform(UNIFORM_LARGURA_RETANGULO, formaRetangulo.getSize().x);
                        shaderNota.setUniform(UNIFORM_ALTURA_RETANGULO, formaRetangulo.getSize().y);
                    }

                    janela.draw(formaRetangulo, estadosRenderizacaoNota);
                }
            }

            // Determina se deve desenhar a cabeça
            bool desenharCapeca = true;
            if (nota.ehNotaLonga && nota.sustainCompleto) {
                const auto comprimentoSustainSec = nota.tempoFimSustainSec - nota.timestampSec;
                const auto pixelsSustain = static_cast<float>(comprimentoSustainSec * VELOCIDADE_QUEDA_NOTA_PPS);
                if (yCentroCapeca - pixelsSustain + alturaCapeca < 0) desenharCapeca = false;
            }

            // Desenha cabeça da nota
            if (desenharCapeca) {
                const auto corCapeca = ((nota.ehNotaLonga && nota.sustainAtivo) || nota.acertada)
                                          ? sf::Color(nota.cor.r, nota.cor.g, nota.cor.b, 255)
                                          : sf::Color(nota.cor.r, nota.cor.g, nota.cor.b, 100);

                formaRetangulo.setSize({larguraVisualCapeca, alturaCapeca});
                formaRetangulo.setPosition({xVisualCapeca, yTopoVisualCapeca});
                formaRetangulo.setFillColor(corCapeca);

                if (spriteBranco) formaRetangulo.setTexture(&spriteBranco->getTexture());
                else formaRetangulo.setTexture(nullptr);

                if (estadosRenderizacaoNota.shader) {
                    shaderNota.setUniform(UNIFORM_LARGURA_RETANGULO, formaRetangulo.getSize().x);
                    shaderNota.setUniform(UNIFORM_ALTURA_RETANGULO, formaRetangulo.getSize().y);
                }

                janela.draw(formaRetangulo, estadosRenderizacaoNota);

                // Desenha marcação de completo para notas longas
                if (nota.ehNotaLonga && nota.acertada && nota.sustainCompleto) {
                    marcacaoCompleto.setFillColor(sf::Color::White);
                    const auto limitesMarcacao = marcacaoCompleto.getLocalBounds();
                    marcacaoCompleto.setOrigin({
                        limitesMarcacao.position.x + limitesMarcacao.size.x / 2.f,
                        limitesMarcacao.position.y + limitesMarcacao.size.y / 2.f
                    });
                    marcacaoCompleto.setPosition({
                        xVisualCapeca + larguraVisualCapeca / 2.f,
                        yCentroCapeca
                    });
                    janela.draw(marcacaoCompleto);
                }
            }
        }
    }
};

// ============================= FUNÇÃO PRINCIPAL =============================

/**
 * @brief Função principal - ponto de entrada do programa
 * @return Código de saída (0 = sucesso)
 */
int main() {
    // Configura locale para UTF-8
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
    std::cerr.imbue(std::locale());

    try {
        Jogo jogo;
        jogo.executar();
    } catch (const std::exception& e) {
        std::cerr << "Erro fatal: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Erro fatal desconhecido!" << std::endl;
        return 1;
    }

    return 0;
}
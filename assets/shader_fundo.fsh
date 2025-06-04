uniform vec2 Resolucao; // Resolução do canvas
uniform float Tempo;    // Tempo global em segundos

void main() {
    // Coordenadas normalizadas (0 a 1) na tela
    vec2 uv = gl_FragCoord.xy / Resolucao.xy;

    // Coordenadas em espaço de pixels para densidade consistente de listras
    vec2 coordsPadrao = gl_FragCoord.xy;

    // Padrão de listras diagonais
    float padraoListra = fract((coordsPadrao.x + coordsPadrao.y) * 0.02);

    // Distância local à borda da listra para suavização
    float localListra = fract(padraoListra * 2.0);
    float distanciaBordaListra = min(localListra, 1.0 - localListra);
    float suavidadeBordaListra = 0.2;
    float fatorSombraListra = smoothstep(0.0, suavidadeBordaListra, distanciaBordaListra);
    float brilhoDaListra = mix(0.7, 1.0, fatorSombraListra);

    // Tons de cinza base para as listras (bem escuros)
    float cinzaBase = 0.15;                 // Nível de cinza base mais escuro
    float listraClara = cinzaBase;          // Tom claro da listra
    float listraEscura = cinzaBase * 0.65;  // Tom escuro da listra (65% do base)

    // Seleciona se a listra atual é clara ou escura
    float cinzaListraAtual = (padraoListra < 0.5) ? listraClara : listraEscura;
    float cinzaDeListras = cinzaListraAtual * brilhoDaListra;

    // Cria efeito de vinheta forte, escurecendo as bordas
    vec2 centralizado = uv - 0.5;
    float raio = length(centralizado);

    // Vinheta simples: escurece baseado na distância ao centro
    float vinheta = 1.0 - (raio * 2.0);
    vinheta = clamp(vinheta, 0.1, 1.0); // Mantém as bordas sem ficar completamente preto

    // Combina as listras com a vinheta
    float cinzaIntermediario = cinzaDeListras * vinheta;

    // Variação sutil baseada no Tempo para dar movimento
    float variacaoTempo = 0.95 + 0.05 * sin(Tempo * 0.5);
    float cinzaFinal = cinzaIntermediario * variacaoTempo;

    // Garante que o valor fique no intervalo válido [0,1]
    cinzaFinal = clamp(cinzaFinal, 0.0, 1.0);

    // Saída de cor em escala de cinza
    gl_FragColor = vec4(vec3(cinzaFinal), 1.0);
}

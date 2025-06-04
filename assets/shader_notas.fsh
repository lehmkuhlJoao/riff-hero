// note_shader_rounded_bevel_ptbr_simplificado.frag

uniform float LarguraRetangulo;    // largura do retângulo em pixels
uniform float AlturaRetangulo;     // altura do retângulo em pixels
uniform float Tempo;               // tempo global para animações (opcional)

const float raioDoCanto = 12.0;    // raio (em pixels) para cantos arredondados

void main()
{
    // posição do fragmento em espaço de pixels dentro do retângulo
    vec2 posFrag = gl_TexCoord[0].xy * vec2(LarguraRetangulo, AlturaRetangulo);

    // determina se está fora dos cantos arredondados usando SDF:
    vec2 d = vec2(
    max(raioDoCanto - posFrag.x, posFrag.x - (LarguraRetangulo - raioDoCanto)),
    max(raioDoCanto - posFrag.y, posFrag.y - (AlturaRetangulo - raioDoCanto))
    );
    if (length(max(d, vec2(0.0))) > raioDoCanto) {
        discard; // descarta fragmentos fora da forma arredondada
    }

    // gera padrão de listras diagonais
    vec2 coordsPadrao = posFrag + vec2(Tempo * 16.0, 0.0);
    float padraoListra = fract((coordsPadrao.x + coordsPadrao.y) * 0.02);
    float localListra = fract(padraoListra * 2.0);
    float distBorda = min(localListra, 1.0 - localListra);
    float brilhoListra = mix(0.7, 1.0, smoothstep(0.0, 0.2, distBorda));

    // aplica cor base e escolhe entre listra clara ou escura
    vec3 corBase = gl_Color.rgb;
    vec3 corDasListras = mix(corBase * 0.65, corBase, step(padraoListra, 0.5)) * brilhoListra;

    // largura do bisel em pixels (baseado na menor dimensão)
    float larguraBiselPx = 0.12 * min(LarguraRetangulo, AlturaRetangulo);

    // distância até o topo, considerando cantos arredondados
    float topDist = posFrag.y;
    if (posFrag.y < raioDoCanto) {
        float cx = clamp(posFrag.x, raioDoCanto, LarguraRetangulo - raioDoCanto);
        topDist = raioDoCanto - length(posFrag - vec2(cx, raioDoCanto));
    }

    // distância até a esquerda, considerando cantos arredondados
    float leftDist = posFrag.x;
    if (posFrag.x < raioDoCanto) {
        float cy = clamp(posFrag.y, raioDoCanto, AlturaRetangulo - raioDoCanto);
        leftDist = raioDoCanto - length(posFrag - vec2(raioDoCanto, cy));
    }

    // distância até o inferior, considerando cantos arredondados
    float bottomDist = AlturaRetangulo - posFrag.y;
    if (posFrag.y > AlturaRetangulo - raioDoCanto) {
        float cx = clamp(posFrag.x, raioDoCanto, LarguraRetangulo - raioDoCanto);
        bottomDist = raioDoCanto - length(posFrag - vec2(cx, AlturaRetangulo - raioDoCanto));
    }

    // distância até a direita, considerando cantos arredondados
    float rightDist = LarguraRetangulo - posFrag.x;
    if (posFrag.x > LarguraRetangulo - raioDoCanto) {
        float cy = clamp(posFrag.y, raioDoCanto, AlturaRetangulo - raioDoCanto);
        rightDist = raioDoCanto - length(posFrag - vec2(LarguraRetangulo - raioDoCanto, cy));
    }

    // cálculo de realce (topo + esquerda)
    float highlight = min(
        clamp((larguraBiselPx - topDist) / larguraBiselPx * 0.20, 0.0, 0.20)
        + clamp((larguraBiselPx - leftDist) / larguraBiselPx * 0.20, 0.0, 0.20),
        0.30
    );

    // cálculo de sombra (inferior + direita)
    float shadow = min(
        clamp((larguraBiselPx - bottomDist) / larguraBiselPx * 0.30, 0.0, 0.30)
        + clamp((larguraBiselPx - rightDist) / larguraBiselPx * 0.30, 0.0, 0.30),
        0.45
    );

    // combinação final: listras + realce - sombra
    vec3 corFinal = clamp(corDasListras + highlight - shadow, 0.0, 1.0);
    gl_FragColor = vec4(corFinal, gl_Color.a);
}

// note_shader.vert (ensure it does this or use SFML's default if you only load fragment)
void main()
{
    // transform the vertex position by the model-view-projection matrix
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // transform the texture coordinates by the texture matrix
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    // forward the vertex color
    gl_FrontColor = gl_Color;
}
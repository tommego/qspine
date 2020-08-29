varying lowp vec4 v_color;
varying mediump vec2 v_texCoord;
uniform sampler2D u_texture;
void main() {
   lowp vec4 t_color = texture2D(u_texture, v_texCoord);
   gl_FragColor = v_color * t_color;
}

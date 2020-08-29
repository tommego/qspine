uniform highp mat4 u_matrix;

varying lowp vec4 v_color;
varying mediump vec2 v_texCoord;

attribute highp vec2 a_position;
attribute mediump vec2 a_texCoord;
attribute lowp vec4 a_color;

void main() {
   gl_Position = u_matrix * vec4(a_position.xy, 0.0, 1.0);
   v_color = a_color;
   v_texCoord = a_texCoord;
}

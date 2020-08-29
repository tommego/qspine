attribute highp vec2 a_position;
uniform mediump vec4 u_color;
uniform highp mat4 u_matrix;
varying mediump vec4 v_color;

void main() {
   gl_Position = u_matrix * vec4(a_position.xy, 0.0, 1.0);
   v_color = u_color;
}

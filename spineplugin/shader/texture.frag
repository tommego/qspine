varying lowp vec4 v_color;
uniform lowp vec4 u_blendColor;
uniform int u_blendColorChannel;
varying mediump vec2 v_texCoord;
uniform sampler2D u_texture;
uniform lowp float u_light;
void main() {
   lowp vec4 t_color = texture2D(u_texture, v_texCoord);
   lowp vec4 ret_color = v_color * t_color;
   if(u_blendColorChannel == 0){
       ret_color = vec4(ret_color.r, ret_color.r, ret_color.r, ret_color.a) * u_blendColor;
   }
   else if(u_blendColorChannel == 1) {
       ret_color = vec4(ret_color.g, ret_color.g, ret_color.g, ret_color.a) * u_blendColor;
   }
   else if(u_blendColorChannel == 2) {
       ret_color = vec4(ret_color.b, ret_color.b, ret_color.b, ret_color.a) * u_blendColor;
   }
   else if(u_blendColorChannel == 3) {
       ret_color = vec4(ret_color.a, ret_color.a, ret_color.a, ret_color.a) * u_blendColor;
   }
   else if(u_blendColorChannel == 4) {
       lowp float gray = ret_color.r * 0.299 + ret_color.g * 0.587 + ret_color.b * 0.114;
       ret_color = vec4(gray, gray, gray, ret_color.a) * u_blendColor;
   }
   ret_color = ret_color * vec4(u_light, u_light, u_light, 1.0);
   ret_color = ret_color * v_color.a; // multiply alpha to filter mess color
   gl_FragColor = ret_color;
}

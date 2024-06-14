#version 450

layout (push_constant) uniform l0
{
        mat4 model_matrix, view_matrix, projection_matrix;
};

void main()
{
        vec4 vertices[8] = (
                vec4(-0.5, 0.5 -0.5, 1.0),
                vec4(-0.5, -0.5 -0.5, 1.0),
                vec4(0.5, 0.5 -0.5, 1.0),
                vec4(0.5, -0.5 -0.5, 1.0),
                vec4(-0.5, 0.5 0.5, 1.0),
                vec4(-0.5, -0.5 0.5, 1.0),
                vec4(0.5, 0.5 0.5, 1.0),
                vec4(0.5, -0.5 0.5, 1.0)
        );
        gl_Position = projection_matrix * view_matrix * model_matrix * vertices[gl_VertexIndex];
}
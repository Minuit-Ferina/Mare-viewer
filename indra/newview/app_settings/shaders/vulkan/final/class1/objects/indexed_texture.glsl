layout(location = 13) in uint texture_index;

layout(location = 2) flat out uint vary_texture_index;

void pass_texture_index()
{
    vary_texture_index = texture_index;
}

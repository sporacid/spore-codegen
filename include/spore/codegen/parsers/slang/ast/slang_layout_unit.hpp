#pragma once

namespace spore::codegen
{
    enum class slang_layout_unit
    {
        none,
        uniform,
        mixed,
        descriptor_table_slot,
        push_constant_buffer,
        varying_input,
        varying_output,
        specialization_constant,
        input_attachment_index,

        d3d_constant_buffer,
        d3d_shader_resource,
        d3d_unordered_access,
        d3d_sampler_state,
#if 0

    None = SLANG_PARAMETER_CATEGORY_NONE,
    Mixed = SLANG_PARAMETER_CATEGORY_MIXED,
    ConstantBuffer = SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER,
    ShaderResource = SLANG_PARAMETER_CATEGORY_SHADER_RESOURCE,
    UnorderedAccess = SLANG_PARAMETER_CATEGORY_UNORDERED_ACCESS,
    VaryingInput = SLANG_PARAMETER_CATEGORY_VARYING_INPUT,
    VaryingOutput = SLANG_PARAMETER_CATEGORY_VARYING_OUTPUT,
    SamplerState = SLANG_PARAMETER_CATEGORY_SAMPLER_STATE,
    Uniform = SLANG_PARAMETER_CATEGORY_UNIFORM,
    DescriptorTableSlot = SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT,
    SpecializationConstant = SLANG_PARAMETER_CATEGORY_SPECIALIZATION_CONSTANT,
    PushConstantBuffer = SLANG_PARAMETER_CATEGORY_PUSH_CONSTANT_BUFFER,
    RegisterSpace = SLANG_PARAMETER_CATEGORY_REGISTER_SPACE,
    GenericResource = SLANG_PARAMETER_CATEGORY_GENERIC,

    RayPayload = SLANG_PARAMETER_CATEGORY_RAY_PAYLOAD,
    HitAttributes = SLANG_PARAMETER_CATEGORY_HIT_ATTRIBUTES,
    CallablePayload = SLANG_PARAMETER_CATEGORY_CALLABLE_PAYLOAD,

    ShaderRecord = SLANG_PARAMETER_CATEGORY_SHADER_RECORD,

    ExistentialTypeParam = SLANG_PARAMETER_CATEGORY_EXISTENTIAL_TYPE_PARAM,
    ExistentialObjectParam = SLANG_PARAMETER_CATEGORY_EXISTENTIAL_OBJECT_PARAM,

    SubElementRegisterSpace = SLANG_PARAMETER_CATEGORY_SUB_ELEMENT_REGISTER_SPACE,

    InputAttachmentIndex = SLANG_PARAMETER_CATEGORY_SUBPASS,

    MetalBuffer = SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER,
    MetalTexture = SLANG_PARAMETER_CATEGORY_METAL_TEXTURE,
    MetalArgumentBufferElement = SLANG_PARAMETER_CATEGORY_METAL_ARGUMENT_BUFFER_ELEMENT,
    MetalAttribute = SLANG_PARAMETER_CATEGORY_METAL_ATTRIBUTE,
    MetalPayload = SLANG_PARAMETER_CATEGORY_METAL_PAYLOAD,

    // DEPRECATED:
    VertexInput = SLANG_PARAMETER_CATEGORY_VERTEX_INPUT,
    FragmentOutput = SLANG_PARAMETER_CATEGORY_FRAGMENT_OUTPUT,
#endif
    };
}
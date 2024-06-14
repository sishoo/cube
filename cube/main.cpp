
#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include "bootstrap/VkBootstrap.h"

#define VK_TRY(try)\
        do\
        {\
                if ((try) != VK_SUCCESS)\
                {\
                        fprintf(stderr, #try" was not VK_SUCCESS");\
                        abort();\
                }\
        } while (0);
        

typedef struct
{
        /* Vulkan core */
        VkInstance instance;
        VkPhysicalDevice pdevice;
        VkDevice ldevice;
        uint32_t queue_family_index;
        VkQueue queue;

        float model_matrix[16];
        float view_matrix[16];
        float projection_matrix[16];

        shaderc_compiler_t shader_compiler;
        uint32_t *vertex_shader_source, vertex_shader_size_bytes, *fragment_shader_source, fragment_shader_size_bytes;
        VkShaderModule vertex_shader_module, fragment_shader_module;

        /* Pipeline */
        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;

        VkSemaphore image_acquired_semaphore;
        uint32_t nswapchain_images;
        VkSwapchainKHR swapchain;
        VkSurfaceKHR surface;
        VkImage *swapchain_images;
        uint32_t previous_resource_index;
        frame_render_infos_t *frame_render_infos;

        /* Pools */
        VkDescriptorPool descriptor_pool;
        VkCommandPool command_pool;

        uint32_t window_width, window_height;
        GLFWwindow *window;

        vkb::Instance vkb_instance;
        vkb::PhysicalDevice vkb_pdevice;
        vkb::Device vkb_ldevice;
        vkb::Swapchain vkb_swapchain;
} cube_renderer_t; 

void cube_renderer_init_instance(cube_renderer_t *const r)
{
        vkb::InstanceBuilder builder;
        auto res = builder
                .set_app_name("Hephaestus Renderer")
                .set_engine_name("Hephaestus Engine")
                .require_api_version(1, 3, 0)
                .use_default_debug_messenger()
              //.set_debug_callback(debug_callback)
                .request_validation_layers()
                .build();
        r->vkb_instance = res.value();
        r->instance = r->vkb_instance.instance;
}

void cube_renderer_init_window(cube_renderer_t *const r, char *const name)
{
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        assert((r->window = glfwCreateWindow(r->window_width, r->window_height, name, NULL, NULL)));
        glfwSetCursorPosCallback(r->window, cube_renderer_window_cursor_pos_callback);
}

void cube_renderer_init_surface(cube_renderer_t *const r)
{
        VK_TRY(glfwCreateWindowSurface(r->instance, r->window, NULL, &r->surface));
}

void cube_renderer_init_pdevice(cube_renderer_t *const r)
{
        #warning DONT DELETE THIS UNTIL YOU FIX THE DAMN PICK FIRST DEVICE UNCONDITIONALLY!!!
        vkb::PhysicalDeviceSelector selector(r->vkb_instance);
        auto res = selector
                       .set_surface(r->surface)
                       .select_first_device_unconditionally()
                       .add_required_extension("VK_KHR_dynamic_rendering")
                       .select();
        r->vkb_pdevice = res.value();
        r->pdevice = r->vkb_pdevice.physical_device;
}

void cube_renderer_find_memory_type_indices(cube_renderer_t *const r)
{
        #warning hardcoded for the windows pc
        r->memory_type_
}

void cube_renderer_init_ldevice(cube_renderer_t *const r)
{
        VkPhysicalDeviceSynchronization2Features sync_features = {};
        sync_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        sync_features.synchronization2 = VK_TRUE;

        vkb::DeviceBuilder builder{r->vkb_pdevice};
        auto res = builder
                       .add_pNext(&sync_features)
                       .build();
        r->vkb_ldevice = res.value();
        r->ldevice = r->vkb_ldevice.device;
}

void cube_renderer_init_queue(cube_renderer_t *const r)
{
        #warning hardcoded for windows pc
        r->queue_family_index = 0;
        vkGetDeviceQueue(r->ldevice, r->queue_family_index, 0, &r->queue);
}

void cube_renderer_init_swapchain(cube_renderer_t *const r)
{
        vkb::SwapchainBuilder builder{r->vkb_ldevice};
        auto res = builder
                       .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                       .build();
        r->vkb_swapchain = res.value();
        r->swapchain = r->vkb_swapchain.swapchain;
}

void cube_renderer_aquire_swapchain_images(cube_renderer_t *const r)
{
        VK_TRY(vkGetSwapchainImagesKHR(r->ldevice, r->swapchain, &r->nswapchain_images, NULL));

        r->swapchain_images = (VkImage *)calloc(sizeof(VkImage), r->nswapchain_images);

        VK_TRY(vkGetSwapchainImagesKHR(r->ldevice, r->swapchain, &r->nswapchain_images, r->swapchain_images));
}

void cube_renderer_init_command_pool(cube_renderer_t *const r)
{
        VkCommandPoolCreateInfo command_pool_create_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = r->queue_family_index
        };

        VK_TRY(vkCreateCommandPool(r->ldevice, &command_pool_create_info, NULL, &r->command_pool));
}

void cube_renderer_init_shader_compiler(cube_renderer_t *const r)
{
        r->shader_compiler = shaderc_compiler_initialize();
}

void cube_renderer_build_shaders(cube_renderer_t *const r)
{
        

        shaderc_compile_options_t compile_options = shaderc_compile_options_initialize();

        /* Preprocess */
        shaderc_compilation_result_t preprocess_result = 
                shaderc_compile_into_preprocessed_text(
                        r->shader_compiler, 
                        src, 
                        status.st_size, 
                        shaderc_compute_shader, 
                        "particle.comp", 
                        "main", 
                        compile_options
                );
        assert(shaderc_result_get_compilation_status(preprocess_result) == shaderc_compilation_status_success);

        /* Compile */
        shaderc_compilation_result_t compile_result = 
                shaderc_compile_into_spv(
                        r->shader_compiler,
                        shaderc_result_get_bytes(preprocess_result), 
                        shaderc_result_get_length(preprocess_result), 
                        shaderc_compute_shader, 
                        "particle.comp", 
                        "main", 
                        compile_options
                );
        assert(shaderc_result_get_compilation_status(compile_result) == shaderc_compilation_status_success);

        r->particle_shader_source = (uint32_t *)shaderc_result_get_bytes(compile_result);
        r->particle_shader_size_bytes = shaderc_result_get_length(compile_result);
}

void cube_renderer_build_pipeline(cube_renderer_t *const r)
{               
        /* NONE!!!! */
        VkPipelineVertexInputStateCreateInfo vertex_input_state = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STAGE_CREATE_INFO
        };

        #warning does it even matter what i put in here????????
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
        };

        VkTesselationStateCreateInfo tesselation_state = {
                .sType = VK_STRUCTURE_TYPE_TESSELATION_STATE_CREATE_INFO 
        };

        VkViewport viewport = {
                .width = 1920.0,
                .height = 1080.0,
                .maxDepth = 1.0
        };
        VkRect2D scissor = {
                .extent.width = 1920,
                .extent.height = 1080
        };
        VkViewportStateCreateInfo viewport_state = {
                .sType = VK_STRUCTURE_TYPE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor
        };      

        VkRasterizationStateCreateInfo rasterization_state = {
                .sType = VK_STRUCTURE_TYPE_RASTERIZATION_STATE_CREATE_INFO,
                .rasterizationDiscardEnabled = VK_TRUE,
                .cullMode = VK_CULL_MODE_NONE,
                #warning if you can see the back of the stuff then change this the front face thing
                .depthBiasEnabled = VK_FALSE,
                .lineWidth = 1.0
        };

        VkGraphicsPipelineCreateInfo pipeline_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO, 
                .stageCount = 2,
                .pStages = shader_stages,
                .pVertexInputState = &vertex_input_state, 
                .pInputAssemblyState = &input_assembly_state,
                .pTesselationState = &tesselation_state,
                .pViewportState = &viewport_state,
                .pRasterizationState = &rasterization_state,
                .
        };

        VK_TRY(vkCreateGraphicsPipelines(r->ldevice, VK_NULL_HANDLE, 1, ))
}


void cube_renderer_init_frame_render_infos(cube_renderer_t *const r)
{
        VkFenceCreateInfo fence_create_info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };

        VkSemaphoreCreateInfo semaphore_create_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkCommandBufferAllocateInfo command_buffer_allocate_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = r->command_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1
        };

        r->frame_render_infos = (frame_render_infos_t *)malloc(sizeof(frame_render_infos_t) * r->nswapchain_images);

        for (uint32_t i = 0; i < r->nswapchain_images; i++)
        {       
                frame_render_infos_t *infos = &r->frame_render_infos[i];
                VK_TRY(vkAllocateCommandBuffers(r->ldevice, &command_buffer_allocate_info, &infos->command_buffer));
                VK_TRY(vkCreateFence           (r->ldevice, &fence_create_info,     NULL,  &infos->complete_fence));
                VK_TRY(vkCreateSemaphore       (r->ldevice, &semaphore_create_info, NULL,  &infos->complete_semaphore));
                VK_TRY(vkCreateSemaphore       (r->ldevice, &semaphore_create_info, NULL,  &infos->image_acquired_semaphore));
        }
}

void cube_renderer_init(cube_renderer_t *const r, char *const window_name, int width, int height)
{
        particles_data_init(&r->particles);

        /* Vulkan Core */
        cube_renderer_init_instance(r);
        r->window_width = width;
        r->window_height = height;
        cube_renderer_init_window(r, window_name);
        cube_renderer_init_surface(r);
        cube_renderer_init_pdevice(r);
        cube_renderer_init_ldevice(r);
        cube_renderer_init_queue(r);
        cube_renderer_init_swapchain(r);
        cube_renderer_aquire_swapchain_images(r);
        cube_renderer_init_command_pool(r);
        cube_renderer_init_descriptor_pool(r);
        cube_renderer_init_shader_compiler(r);

        /* Cube Specifics */
        cube_renderer_init_frame_render_infos(r);
        cube_renderer_build_shaders(r);
        cube_renderer_build_pipeline(r);

}

void cube_renderer_draw(cube_renderer_t *const r)
{

        uint32_t resource_index = r->previous_resource_index + 1;

        if (resource_index)
        {
                resource_index = 0;
        }

        frame_render_infos_t frame_infos = r->frame_render_infos[resource_index];

        VK_TRY(vkWaitForFences(r->ldevice, ));
        VK_TRY(vkResetFence(r->ldevice, ));

        VkCommandBufferBeginInfo begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        VK_TRY(vkBeginCommandBuffer(frame_infos.command_buffer, &begin_info));

        vkCmdBindPipeline(frame_infos.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHCS, r->pipeline);
        
        vkCmdPushConstants(
                frame_infos.command_buffer,     
                r->pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof r->push_constant_block,
                &r->push_constant_block
        );

        vkCmdDraw(frame_infos.command_buffer, 8, 1, 0, 0);

        VK_TRY(vkEndCommandBuffer(frame_infos.command_buffer));
}

int main()
{
        cube_renderer_t renderer = {};
        cube_renderer_init(&renderer, "CUBOID CUBE", 800, 600);

        while (!glfwShouldClose(renderer.window))
        {
                cube_renderer_draw(&renderer);
        }


        return 0;
}
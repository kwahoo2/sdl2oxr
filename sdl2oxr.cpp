//
//  Created by Bradley Austin Davis on 2019/09/18
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#define XR_USE_GRAPHICS_API_OPENGL//needed for xr::GraphicsRequirementsOpenGLKHR and xr::SwapchainImageOpenGLKHR
#include <glad/glad.h> //glad before glx.h and glx.h before openxr_platform.h!
#if defined(WIN32)
#define XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__ANDROID__)
#define XR_USE_PLATFORM_ANDROID
#elif defined(__linux__)
#define XR_USE_PLATFORM_XLIB
#include <X11/Xlib.h> //linux specific
#include <GL/glx.h>
#endif

#include <cstdint>
#include <unordered_map>
#include <functional>

#include <openxr/openxr.hpp>
#include <openxr/openxr_platform.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL2/SDL.h>

#include <glad/glad.h>
#include <logging.hpp>

namespace xrs {

namespace DebugUtilsEXT {

using MessageSeverityFlagBits = xr::DebugUtilsMessageSeverityFlagBitsEXT;
using MessageTypeFlagBits = xr::DebugUtilsMessageTypeFlagBitsEXT;
using MessageSeverityFlags = xr::DebugUtilsMessageSeverityFlagsEXT;
using MessageTypeFlags = xr::DebugUtilsMessageTypeFlagsEXT;
using CallbackData = xr::DebugUtilsMessengerCallbackDataEXT;
using Messenger = xr::DebugUtilsMessengerEXT;

// Raw C callback
static XrBool32 debugCallback(XrDebugUtilsMessageSeverityFlagsEXT sev_,
                              XrDebugUtilsMessageTypeFlagsEXT type_,
                              const XrDebugUtilsMessengerCallbackDataEXT* data_,
                              void* userData) {
    LOG_FORMATTED((logging::Level)sev_, "{}: {}", data_->functionName, data_->message);
    return XR_TRUE;
}

Messenger create(const xr::Instance& instance,
                 const MessageSeverityFlags& severityFlags = MessageSeverityFlagBits::AllBits,
                 const MessageTypeFlags& typeFlags = MessageTypeFlagBits::AllBits,
                 void* userData = nullptr) {
    return instance.createDebugUtilsMessengerEXT({ severityFlags, typeFlags, debugCallback, userData },
                                                 xr::DispatchLoaderDynamic{ instance });
}

}  // namespace DebugUtilsEXT

inline XrFovf toTanFovf(const XrFovf& fov) {
    return { tanf(fov.angleLeft), tanf(fov.angleRight), tanf(fov.angleUp), tanf(fov.angleDown) };
}

inline glm::mat4 toGlm(const XrFovf& fov, float nearZ = 0.01f, float farZ = 10000.0f) {
    auto tanFov = toTanFovf(fov);
    const auto& tanAngleRight = tanFov.angleRight;
    const auto& tanAngleLeft = tanFov.angleLeft;
    const auto& tanAngleUp = tanFov.angleUp;
    const auto& tanAngleDown = tanFov.angleDown;

    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = (tanAngleDown - tanAngleUp);
    const float offsetZ = 0;

    glm::mat4 resultm{};
    float* result = &resultm[0][0];
    // normal projection
    result[0] = 2 / tanAngleWidth;
    result[4] = 0;
    result[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    result[12] = 0;

    result[1] = 0;
    result[5] = 2 / tanAngleHeight;
    result[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    result[13] = 0;

    result[2] = 0;
    result[6] = 0;
    result[10] = -(farZ + offsetZ) / (farZ - nearZ);
    result[14] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

    result[3] = 0;
    result[7] = 0;
    result[11] = -1;
    result[15] = 0;

    return resultm;
}

inline glm::quat toGlm(const XrQuaternionf& q) {
    return glm::make_quat(&q.x);
}

inline glm::vec3 toGlm(const XrVector3f& v) {
    return glm::make_vec3(&v.x);
}

inline glm::mat4 toGlm(const XrPosef& p) {
    glm::mat4 orientation = glm::mat4_cast(toGlm(p.orientation));
    glm::mat4 translation = glm::translate(glm::mat4{ 1 }, toGlm(p.position));
    return translation * orientation;
}

}  // namespace xrs

inline void debugMessageCallback(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar* message,
                                 const void* userParam) {
    std::cout << message << std::endl;
}

static std::string formatToString(GLenum format) {
    switch (format) {
        case GL_COMPRESSED_R11_EAC:
            return "COMPRESSED_R11_EAC";
        case GL_COMPRESSED_RED_RGTC1:
            return "COMPRESSED_RED_RGTC1";
        case GL_COMPRESSED_RG_RGTC2:
            return "COMPRESSED_RG_RGTC2";
        case GL_COMPRESSED_RG11_EAC:
            return "COMPRESSED_RG11_EAC";
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            return "COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT";
        case GL_COMPRESSED_RGB8_ETC2:
            return "COMPRESSED_RGB8_ETC2";
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
            return "COMPRESSED_RGBA8_ETC2_EAC";
        case GL_COMPRESSED_SIGNED_R11_EAC:
            return "COMPRESSED_SIGNED_R11_EAC";
        case GL_COMPRESSED_SIGNED_RG11_EAC:
            return "COMPRESSED_SIGNED_RG11_EAC";
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return "COMPRESSED_SRGB_ALPHA_BPTC_UNORM";
        case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            return "COMPRESSED_SRGB8_ALPHA8_ETC2_EAC";
        case GL_COMPRESSED_SRGB8_ETC2:
            return "COMPRESSED_SRGB8_ETC2";
        case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case GL_DEPTH_COMPONENT16:
            return "DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24:
            return "DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32:
            return "DEPTH_COMPONENT32";
        case GL_DEPTH_COMPONENT32F:
            return "DEPTH_COMPONENT32F";
        case GL_DEPTH24_STENCIL8:
            return "DEPTH24_STENCIL8";
        case GL_R11F_G11F_B10F:
            return "R11F_G11F_B10F";
        case GL_R16_SNORM:
            return "R16_SNORM";
        case GL_R16:
            return "R16";
        case GL_R16F:
            return "R16F";
        case GL_R16I:
            return "R16I";
        case GL_R16UI:
            return "R16UI";
        case GL_R32F:
            return "R32F";
        case GL_R32I:
            return "R32I";
        case GL_R32UI:
            return "R32UI";
        case GL_R8_SNORM:
            return "R8_SNORM";
        case GL_R8:
            return "R8";
        case GL_R8I:
            return "R8I";
        case GL_R8UI:
            return "R8UI";
        case GL_RG16_SNORM:
            return "RG16_SNORM";
        case GL_RG16:
            return "RG16";
        case GL_RG16F:
            return "RG16F";
        case GL_RG16I:
            return "RG16I";
        case GL_RG16UI:
            return "RG16UI";
        case GL_RG32F:
            return "RG32F";
        case GL_RG32I:
            return "RG32I";
        case GL_RG32UI:
            return "RG32UI";
        case GL_RG8_SNORM:
            return "RG8_SNORM";
        case GL_RG8:
            return "RG8";
        case GL_RG8I:
            return "RG8I";
        case GL_RG8UI:
            return "RG8UI";
        case GL_RGB10_A2:
            return "RGB10_A2";
        case GL_RGB8:
            return "RGB8";
        case GL_RGB9_E5:
            return "RGB9_E5";
        case GL_RGBA16_SNORM:
            return "RGBA16_SNORM";
        case GL_RGBA16:
            return "RGBA16";
        case GL_RGBA16F:
            return "RGBA16F";
        case GL_RGBA16I:
            return "RGBA16I";
        case GL_RGBA16UI:
            return "RGBA16UI";
        case GL_RGBA2:
            return "RGBA2";
        case GL_RGBA32F:
            return "RGBA32F";
        case GL_RGBA32I:
            return "RGBA32I";
        case GL_RGBA32UI:
            return "RGBA32UI";
        case GL_RGBA8_SNORM:
            return "RGBA8_SNORM";
        case GL_RGBA8:
            return "RGBA8";
        case GL_RGBA8I:
            return "RGBA8I";
        case GL_RGBA8UI:
            return "RGBA8UI";
        case GL_SRGB8_ALPHA8:
            return "SRGB8_ALPHA8";
        case GL_SRGB8:
            return "SRGB8";
        case GL_RGB16F:
            return "RGB16F";
        case GL_DEPTH32F_STENCIL8:
            return "DEPTH32F_STENCIL8";
        case GL_BGR:
            return "BGR (Out of spec)";
        case GL_BGRA:
            return "BGRA (Out of spec)";
    }
    return "unknown";
}

struct OpenXrExample {
    bool quit{ false };

    // Application main function
    void run() {
        // Startup work
        prepare();

        // Loop
        while (!quit) {
            frame();
        }

        // Teardown work
        destroy();
    }

    //////////////////////////////////////
    // One-time setup work              //
    //////////////////////////////////////

    // The top level prepare function, which is broken down by task
    void prepare() {
        // The OpenXR instance and the OpenXR system provide information we'll require to create our window
        // and rendering backend, so it has to come first
        prepareXrInstance();
        prepareXrSystem();

        prepareWindow();
        prepareXrSession();
        prepareXrSwapchain();
        prepareXrCompositionLayers();
        prepareGlFramebuffer();
    }

    bool enableDebug{ true };
    xr::Instance instance;
    xr::DispatchLoaderDynamic dispatch;
    xrs::DebugUtilsEXT::Messenger messenger;
    void prepareXrInstance() {
        std::unordered_map<std::string, xr::ExtensionProperties> discoveredExtensions;
        for (const auto& extensionProperties : xr::enumerateInstanceExtensionProperties(nullptr)) {
            discoveredExtensions.insert({ extensionProperties.extensionName, extensionProperties });
        }

#if !defined(SUPPRESS_DEBUG_UTILS)
        if (0 == discoveredExtensions.count(XR_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            enableDebug = false;
        }
#else
        enableDebug = false;
#endif

        std::vector<const char*> requestedExtensions;
        if (0 == discoveredExtensions.count(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)) {
            throw std::runtime_error(
                fmt::format("Required Graphics API extension not available: {}", XR_KHR_OPENGL_ENABLE_EXTENSION_NAME));
        }
        requestedExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

        if (enableDebug) {
            requestedExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        xr::InstanceCreateInfo ici{ {},
                                    { "gl_single_file_example", 0, "openXrSamples", 0, xr::Version::current() },
                                    0,
                                    nullptr,
                                    (uint32_t)requestedExtensions.size(),
                                    requestedExtensions.data() };

        xr::DebugUtilsMessengerCreateInfoEXT dumci;
        if (enableDebug) {
            dumci.messageSeverities = xr::DebugUtilsMessageSeverityFlagBitsEXT::AllBits;
            dumci.messageTypes = xr::DebugUtilsMessageTypeFlagBitsEXT::AllBits;
            dumci.userData = this;
            dumci.userCallback = &xrs::DebugUtilsEXT::debugCallback;
            ici.next = &dumci;
        }

        // Create the actual instance
        instance = xr::createInstance(ici);

        // Turn on debug logging
        if (enableDebug) {
            messenger = xrs::DebugUtilsEXT::create(instance);
        }

        // Having created the isntance, the very first thing to do is populate the dynamic dispatch, loading
        // all the available functions from the runtime
        dispatch = xr::DispatchLoaderDynamic::createFullyPopulated(instance, &xrGetInstanceProcAddr);

        // Log the instance properties
        xr::InstanceProperties instanceProperties = instance.getInstanceProperties();
        LOG_INFO("OpenXR Runtime {} version {}.{}.{}",  //
                 (const char*)instanceProperties.runtimeName, instanceProperties.runtimeVersion.major(),
                 instanceProperties.runtimeVersion.minor(), instanceProperties.runtimeVersion.patch());
    }

    xr::SystemId systemId;
    glm::uvec2 renderTargetSize;
    xr::GraphicsRequirementsOpenGLKHR graphicsRequirements;
    void prepareXrSystem() {
        // We want to create an HMD example, so we ask for a runtime that supposts that form factor
        // and get a response in the form of a systemId
        systemId = instance.getSystem(xr::SystemGetInfo{ xr::FormFactor::HeadMountedDisplay });

        // Log the system properties
        {
            xr::SystemProperties systemProperties = instance.getSystemProperties(systemId);
            LOG_INFO("OpenXR System {} max layers {} max swapchain image size {}x{}",  //
                     (const char*)systemProperties.systemName, (uint32_t)systemProperties.graphicsProperties.maxLayerCount,
                     (uint32_t)systemProperties.graphicsProperties.maxSwapchainImageWidth,
                     (uint32_t)systemProperties.graphicsProperties.maxSwapchainImageHeight);
        }

        // Find out what view configurations we have available
        {
            auto viewConfigTypes = instance.enumerateViewConfigurations(systemId);
            auto viewConfigType = viewConfigTypes[0];
            if (viewConfigType != xr::ViewConfigurationType::PrimaryStereo) {
                throw std::runtime_error("Example only supports stereo-based HMD rendering");
            }
            //xr::ViewConfigurationProperties viewConfigProperties =
            //    instance.getViewConfigurationProperties(systemId, viewConfigType);
            //logging::log(logging::Level::Info, fmt::format(""));
        }

        std::vector<xr::ViewConfigurationView> viewConfigViews =
            instance.enumerateViewConfigurationViews(systemId, xr::ViewConfigurationType::PrimaryStereo);

        // Instead of createing a swapchain per-eye, we create a single swapchain of double width.
        // Even preferable would be to create a swapchain texture array with one layer per eye, so that we could use the
        // VK_KHR_multiview to render both eyes with a single set of draws, but sadly the Oculus runtime doesn't currently
        // support texture array swapchains
        // one swapchain per eye
        if (viewConfigViews.size() != 2) {
            throw std::runtime_error("Unexpected number of view configurations");
        }

        if (viewConfigViews[0].recommendedImageRectHeight != viewConfigViews[1].recommendedImageRectHeight) {
            throw std::runtime_error("Per-eye images have different recommended heights");
        }

        renderTargetSize = { viewConfigViews[0].recommendedImageRectWidth, viewConfigViews[0].recommendedImageRectHeight };

        graphicsRequirements = instance.getOpenGLGraphicsRequirementsKHR(systemId, dispatch);
    }

    SDL_Window* window;
    SDL_GLContext context;
    glm::uvec2 windowSize;
    void prepareWindow() {
        assert(renderTargetSize.x != 0 && renderTargetSize.y != 0);
        windowSize = renderTargetSize;
        windowSize /= 4;

        SDL_Init(SDL_INIT_VIDEO);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, graphicsRequirements.maxApiVersionSupported.major());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, graphicsRequirements.maxApiVersionSupported.minor());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

        window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSize.x, windowSize.y,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, context);
        SDL_GL_SetSwapInterval(0);
        gladLoadGL();
        glDebugMessageCallback(debugMessageCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

    xr::Session session;
    void prepareXrSession() {
#if defined XR_USE_PLATFORM_WIN32
        xr::GraphicsBindingOpenGLWin32KHR graphicsBinding{ wglGetCurrentDC(), wglGetCurrentContext() };
#endif
        //https://www.khronos.org/registry/OpenXR/specs/1.0/man/html/XrGraphicsBindingOpenGLXlibKHR.html
#if defined XR_USE_PLATFORM_XLIB
        Display* xDisplay;
        int xScreen;
        uint32_t visualid;
        GLXFBConfig glxFBConfig;
        GLXDrawable glxDrawable;
        GLXContext glxContext;
        
        xDisplay = XOpenDisplay(NULL);
        xScreen = XDefaultScreen(xDisplay);
        glxContext = glXGetCurrentContext();
        glxDrawable = glXGetCurrentDrawable();
        
        /*To render using OpenGL into a GLX drawable, you must determine the appropriate GLXFBConfig that supports the rendering features your application requires. glXChooseFBConfig returns a GLXFBConfig matching the required attributes or NULL if no match is found. A complete list of GLXFBConfigs supported by a server can be obtained by calling glXGetFBConfigs. Attributes of a particular GLXFBConfig can be queried by calling glXGetFBConfigAttrib. */
        int fbConfigCount = 0;
        GLXFBConfig *fbConfigs = glXGetFBConfigs(xDisplay, xScreen, &fbConfigCount);
        
        if (fbConfigCount == 0) {
            throw std::runtime_error("No valid framebuffer configurations found");
        }
        
        int value;
        for (int i = 0; i < fbConfigCount; i++) {
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_FBCONFIG_ID, &value);
            if (value == 0) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_VISUAL_ID, &value);
            if (value == 0) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_DOUBLEBUFFER, &value);
            if (value == 0) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_RENDER_TYPE, &value);
            if ((value & GLX_RGBA_BIT) == 0) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_DRAWABLE_TYPE, &value);
            if ((value & GLX_WINDOW_BIT) == 0) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_RED_SIZE, &value);
            if (value != 8) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_GREEN_SIZE, &value);
            if (value != 8) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_BLUE_SIZE, &value);
            if (value != 8) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_ALPHA_SIZE, &value);
            if (value != 8) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_DEPTH_SIZE, &value);
            if (value != 24) {
                continue;
            }
            glXGetFBConfigAttrib(xDisplay, fbConfigs[i], GLX_VISUAL_ID, &value);
            visualid = value;
            glxFBConfig = fbConfigs[i];
            LOG_INFO("Visual id: {}",  value);
            break;
        }
        
        xr::GraphicsBindingOpenGLXlibKHR graphicsBinding { xDisplay,
                                                           visualid,
                                                           glxFBConfig,
                                                           glxDrawable,
                                                           glxContext };
#endif        
        
        xr::SessionCreateInfo sci{ {}, systemId };
        sci.next = &graphicsBinding;
        session = instance.createSession(sci);

        auto referenceSpaces = session.enumerateReferenceSpaces();
        space = session.createReferenceSpace(xr::ReferenceSpaceCreateInfo{ xr::ReferenceSpaceType::Local });

        auto swapchainFormats = session.enumerateSwapchainFormats();
        for (const auto& format : swapchainFormats) {
            LOG_INFO("\t{}", formatToString((GLenum)format));
        }
    }

    xr::SwapchainCreateInfo swapchainCreateInfo;
    std::vector<xr::Swapchain> swapchains; //one swapchain per eye
    std::vector<std::vector<xr::SwapchainImageOpenGLKHR> > swapchainImagesArr; //two swapchain images (for both eyes)
    void prepareXrSwapchain() {
        xr::for_each_side_index([&](uint32_t eyeIndex){
            xr::Swapchain swapchain;
            std::vector<xr::SwapchainImageOpenGLKHR> swapchainImages;

            swapchainCreateInfo.usageFlags = xr::SwapchainUsageFlagBits::TransferDst;
            swapchainCreateInfo.format = (int64_t)GL_SRGB8_ALPHA8;
            swapchainCreateInfo.sampleCount = 1;
            swapchainCreateInfo.arraySize = 1;
            swapchainCreateInfo.faceCount = 1;
            swapchainCreateInfo.mipCount = 1;
            swapchainCreateInfo.width = renderTargetSize.x;
            swapchainCreateInfo.height = renderTargetSize.y;
            swapchain = session.createSwapchain(swapchainCreateInfo);
            swapchainImages = swapchain.enumerateSwapchainImages<xr::SwapchainImageOpenGLKHR>();

            swapchains.push_back(swapchain);
            swapchainImagesArr.push_back(swapchainImages);
        });
    }

    std::array<xr::CompositionLayerProjectionView, 2> projectionLayerViews;
    xr::CompositionLayerProjection projectionLayer{ {}, {}, 2, projectionLayerViews.data() };
    xr::Space& space{ projectionLayer.space };
    std::vector<xr::CompositionLayerBaseHeader*> layersPointers;
    void prepareXrCompositionLayers() {
        //session.getReferenceSpaceBoundsRect(xr::ReferenceSpaceType::Local, bounds);
        projectionLayer.viewCount = 2;
        projectionLayer.views = projectionLayerViews.data();
        layersPointers.push_back(&projectionLayer);
        // Finish setting up the layer submission
        xr::for_each_side_index([&](uint32_t eyeIndex) {
            auto& layerView = projectionLayerViews[eyeIndex];
            layerView.subImage.swapchain = swapchains[eyeIndex];
            layerView.subImage.imageRect.offset = {0, 0};
            layerView.subImage.imageRect.extent = { (int32_t)renderTargetSize.x, (int32_t)renderTargetSize.y };
        });
    }

    struct GLFBO {
        GLuint id{ 0 };
        GLuint depthBuffer{ 0 };
    } fbo;

    void prepareGlFramebuffer() {
        // Create a depth renderbuffer compatible with the Swapchain sample count and size
        glGenRenderbuffers(1, &fbo.depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, fbo.depthBuffer);
        if (swapchainCreateInfo.sampleCount == 1) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, swapchainCreateInfo.width, swapchainCreateInfo.height);
        } else {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, swapchainCreateInfo.sampleCount, GL_DEPTH24_STENCIL8,
                                             swapchainCreateInfo.width, swapchainCreateInfo.height);
        }

        // Create a framebuffer and attach the depth buffer to it
        glGenFramebuffers(1, &fbo.id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.id);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo.depthBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    //////////////////////////////////////
    // Per-frame work                   //
    //////////////////////////////////////
    void frame() {
        pollSdlEvents();
        pollXrEvents();
        if (quit) {
            return;
        }
        if (startXrFrame()) {
            updateXrViews();
            if (frameState.shouldRender) {
                render();
            }
            endXrFrame();
        }
    }

    void pollSdlEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYUP:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    }
                    break;
            }
        }
    }

    void pollXrEvents() {
        while (true) {
            xr::EventDataBuffer eventBuffer;
            auto pollResult = instance.pollEvent(eventBuffer);
            if (pollResult == xr::Result::EventUnavailable) {
                break;
            }

            switch (eventBuffer.type) {
                case xr::StructureType::EventDataSessionStateChanged:
                    onSessionStateChanged(reinterpret_cast<xr::EventDataSessionStateChanged&>(eventBuffer));
                    break;

                default:
                    break;
            }
        }
    }

    xr::SessionState sessionState{ xr::SessionState::Idle };
    void onSessionStateChanged(const xr::EventDataSessionStateChanged& sessionStateChangedEvent) {
        sessionState = sessionStateChangedEvent.state;
        switch (sessionState) {
            case xr::SessionState::Ready:
                if (!quit) {
                    session.beginSession(xr::SessionBeginInfo{ xr::ViewConfigurationType::PrimaryStereo });
                }
                break;

            case xr::SessionState::Stopping:
                session.endSession();
                quit = true;
                break;

            default:
                break;
        }
    }

    xr::FrameState frameState;
    bool startXrFrame() {
        switch (sessionState) {
            case xr::SessionState::Ready:
            case xr::SessionState::Focused:
            case xr::SessionState::Synchronized:
            case xr::SessionState::Visible:
                session.waitFrame(xr::FrameWaitInfo{}, frameState);
                return xr::Result::Success == session.beginFrame(xr::FrameBeginInfo{});

            default:
                break;
        }

        return false;
    }

    void endXrFrame() {
        xr::FrameEndInfo frameEndInfo{ frameState.predictedDisplayTime, xr::EnvironmentBlendMode::Opaque };
        if (frameState.shouldRender) {
            xr::for_each_side_index([&](uint32_t eyeIndex) {
                auto& layerView = projectionLayerViews[eyeIndex];
                const auto& eyeView = eyeViewStates[eyeIndex];
                layerView.fov = eyeView.fov;
                layerView.pose = eyeView.pose;
            });
            frameEndInfo.layerCount = (uint32_t)layersPointers.size();
            frameEndInfo.layers = layersPointers.data();
        }
        session.endFrame(frameEndInfo);
    }

    std::vector<xr::View> eyeViewStates;
    std::array<glm::mat4, 2> eyeViews;
    std::array<glm::mat4, 2> eyeProjections;
    void updateXrViews() {
        xr::ViewState vs;
        xr::ViewLocateInfo vi{ xr::ViewConfigurationType::PrimaryStereo, frameState.predictedDisplayTime, space };
        eyeViewStates = session.locateViews(vi, &(vs.operator XrViewState&()));
        xr::for_each_side_index([&](size_t eyeIndex) {
            const auto& viewState = eyeViewStates[eyeIndex];
            eyeProjections[eyeIndex] = xrs::toGlm(viewState.fov);
            eyeViews[eyeIndex] = glm::inverse(xrs::toGlm(viewState.pose));
        });
    }

    void render() {
        xr::for_each_side_index([&](uint32_t eyeIndex){

            uint32_t swapchainIndex;
            xr::Swapchain swapchain = swapchains[eyeIndex];

            swapchain.acquireSwapchainImage(xr::SwapchainImageAcquireInfo{}, &swapchainIndex);
            swapchain.waitSwapchainImage(xr::SwapchainImageWaitInfo{ xr::Duration::infinite() });

            glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);
            //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, swapchainImages[swapchainIndex].image, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, swapchainImagesArr[eyeIndex][swapchainIndex].image, 0);

            // "render" to the swapchain image
            if (eyeIndex == 0)
            {
                glClearColor(0, 1, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);// draw once per eye - left
            }
            else
            {
                glClearColor(0, 0, 1, 1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                // fast blit from the fbo to the window surface
                /*glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glBlitFramebuffer(0, 0, renderTargetSize.x, renderTargetSize.y, 0, 0, windowSize.x, windowSize.y, GL_COLOR_BUFFER_BIT,
                                GL_NEAREST);*/

                glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            }

            swapchain.releaseSwapchainImage(xr::SwapchainImageReleaseInfo{});

        });

        SDL_GL_SwapWindow(window);
    }

    //////////////////////////////////////
    // Shutdown                         //
    //////////////////////////////////////
    void destroy() {
        if (fbo.id != 0) {
            glDeleteFramebuffers(1, &fbo.id);
            fbo.id = 0;
        }

        if (fbo.depthBuffer != 0) {
            glDeleteRenderbuffers(1, &fbo.depthBuffer);
            fbo.depthBuffer = 0;
        }

        for (xr::Swapchain swapchain : swapchains) {
            if (swapchain)
            {
                swapchain.destroy();
                swapchain = nullptr;
            }
        }
        if (session) {
            session.destroy();
            session = nullptr;
        }

        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);

        if (messenger) {
            messenger.destroy(dispatch);
        }
        if (instance) {
            instance.destroy();
            instance = nullptr;
        }

        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    try {
        OpenXrExample().run();
    } catch (const std::exception& err) {
        logging::log(logging::Level::Error, err.what());
    }
    return 0;
}

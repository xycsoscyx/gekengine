#include "GEK\Math\Common.hpp"
#include "GEK\Math\Float4x4.hpp"
#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Engine\Population.hpp"
#include <Windows.h>

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Gek;

struct Parameters
{
    float feetPerUnit;
};

void getLights(const Parameters &parameters, const aiScene *scene, const aiNode *node, const Math::Float4x4 &accumulatedTransform, std::map<StringUTF8, const aiLight *> &lights)
{
    if (node == nullptr)
    {
        throw std::exception("Invalid model node");
    }

    Math::Float4x4 localTransform(&node->mTransformation.a1);
    Math::Float4x4 nodeTransform(localTransform.getTranspose() * accumulatedTransform);
    auto lightSearch = lights.find(node->mName.C_Str());
    if (lightSearch != lights.end())
    {
        const aiLight *light = lightSearch->second;
        Math::Float3 position((nodeTransform * Math::Float3(light->mPosition.x, light->mPosition.y, light->mPosition.z).w(1.0f)).xyz * parameters.feetPerUnit);
        Math::Quaternion rotation(nodeTransform.getQuaternion());

        printf("--- constant: %f, linear: %f, quadradic: %f\r\n", light->mAttenuationConstant, light->mAttenuationLinear, light->mAttenuationQuadratic);

        printf("<entity name=\"%s\">\r\n", light->mName.C_Str());
        printf("    <transform position=\"(%f, %f, %f)\" rotation=\"(%f, %f, %f, %f)\" />\r\n",
            position.x, position.y, position.z,
            rotation.x, rotation.y, rotation.z, rotation.w);

        float range = 10.0f;

        switch (light->mType)
        {
        case aiLightSource_DIRECTIONAL:
            printf("    <directional_light />\r\n");
            break;

        case aiLightSource_POINT:
            printf("    <point_light range=\"%f\" />\r\n", range);
            break;

        case aiLightSource_SPOT:
            printf("    <spot_light range=\"%f\" inner_angle=\"%f\" outer_angle=\"%f\" falloff=\"5\" />\r\n", range, 
                Math::convertRadiansToDegrees(light->mAngleInnerCone), 
                Math::convertRadiansToDegrees(light->mAngleOuterCone));
            break;

        case aiLightSource_AMBIENT:
            break;

        case aiLightSource_AREA:
            break;

        default:
            break;
        };

        Math::Float3 color(Math::Float3(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b) * 0.01f);
        printf("    <color>(%f, %f, %f)</color>\r\n", color.x, color.y, color.z);
        printf("</entity>\r\n");
    }

    if (node->mNumChildren > 0)
    {
        if (node->mChildren == nullptr)
        {
            throw std::exception("Invalid child list");
        }

        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            getLights(parameters, scene, node->mChildren[childIndex], nodeTransform, lights);
        }
    }
}

int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    CoInitialize(nullptr);
    try
    {
        printf("GEK Scene Converter\r\n");

        String fileNameInput;
        String fileNameOutput;
        Parameters parameters;
        parameters.feetPerUnit = 1.0f;
        for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
        {
            String argument(argumentList[argumentIndex]);
            std::vector<String> arguments(argument.split(L':'));
            if (arguments.empty())
            {
                throw std::exception("No arguments specified for command line parameter");
            }

            if (arguments[0].compareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-unitsInFoot") == 0)
            {
                if (arguments.size() != 2)
                {
                    throw std::exception("Missing parameters for unitsInFoot");
                }

                parameters.feetPerUnit = (1.0f / (float)arguments[1]);
            }
        }

        aiLogStream logStream;
        logStream.callback = [](const char *message, char *user) -> void
        {
            printf("Assimp: %s", message);
        };

        logStream.user = nullptr;
        aiAttachLogStream(&logStream);

        int notRequiredComponents =
            aiComponent_TEXCOORDS |
            aiComponent_NORMALS |
            aiComponent_TANGENTS_AND_BITANGENTS |
            aiComponent_COLORS |
            aiComponent_BONEWEIGHTS |
            aiComponent_ANIMATIONS |
            0;

        unsigned int importFlags =
            aiProcess_RemoveComponent | // remove extra data that is not required
            0;

        unsigned int postProcessFlags =
            aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
            0;

        aiPropertyStore *propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);

        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
        auto originalScene = aiImportFileExWithProperties(StringUTF8(fileNameInput), importFlags, nullptr, propertyStore);
        if (originalScene == nullptr)
        {
            throw std::exception("Unable to load scene with Assimp");
        }

        auto scene = aiApplyPostProcessing(originalScene, postProcessFlags);
        if (scene == nullptr)
        {
            throw std::exception("Unable to apply post processing with Assimp");
        }

        if(scene->HasLights())
        {
            std::map<StringUTF8, const aiLight *> lights;
            for (uint32_t lightIndex = 0; lightIndex < scene->mNumLights; ++lightIndex)
            {
                const aiLight *light = scene->mLights[lightIndex];
                lights[light->mName.C_Str()] = light;
            }

            printf("\r\n");
            getLights(parameters, scene, scene->mRootNode, Math::Float4x4::Identity, lights);
        }

        aiReleaseImport(scene);
    }
    catch (const std::exception &exception)
    {
        printf("[error] Exception occurred: %s", exception.what());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    CoUninitialize();
    return 0;
}
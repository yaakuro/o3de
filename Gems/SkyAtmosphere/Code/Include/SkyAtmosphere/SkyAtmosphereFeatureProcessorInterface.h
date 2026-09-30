/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Instance/Instance.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace RPI
    {
        class Image;
        class RenderPipeline;
    } // namespace RPI
} // namespace AZ

namespace SkyAtmosphere
{
    struct SkyAtmosphereParams
    {
        //! Params that are used for LUT generation
        AZ::Vector3 m_luminanceFactor;
        AZ::Vector3 m_rayleighScattering;
        AZ::Vector3 m_mieScattering;
        AZ::Vector3 m_mieAbsorption;
        AZ::Vector3 m_absorption;
        AZ::Vector3 m_groundAlbedo;
        float m_rayleighExpDistribution;
        float m_mieExpDistribution;
        float m_planetRadius;
        float m_atmosphereRadius;

        //! General params
        AZ::Vector3 m_planetOrigin;
        uint8_t m_minSamples;
        uint8_t m_maxSamples;
        AZ::Vector3 m_sunDirection;
        AZ::Color m_sunColor;
        AZ::Color m_sunLimbColor;
        float m_sunFalloffFactor;
        float m_sunRadiusFactor;
        float m_sunShadowsFarClip;
        float m_nearClip;
        float m_nearFadeDistance;
        float m_aerialDepthFactor;
        bool m_shadowsEnabled;
        bool m_sunEnabled;
        bool m_fastSkyEnabled;
        bool m_fastAerialPerspectiveEnabled;
        bool m_aerialPerspectiveEnabled;
        bool m_multipleScatteringEnabled;

        //! LUT resolution params
        uint16_t m_skyViewLutWidth = 192;
        uint16_t m_skyViewLutHeight = 108;
        uint16_t m_volumeLutDim = 32; // must be a multiple of 32
        float m_volumeKmPerSlice = 4.0f;
        float m_exposure = 20.0f;
    };

    //! Selects one of the images the atmosphere pass precomputes each time its parameters change.
    enum class AtmosphereLUTType
    {
        //! 256x64 R16G16B16A16 transmittance. x = sun zenith cosine, y = view height (both remapped
        //! to 0..1). Sample with LutTransmittanceParamsToUv() from SkyAtmosphereCommon.azsli.
        Transmittance,

        //! Sky colour by view direction (R11G11B10, default 192x108).
        SkyView,

        //! 3D aerial-perspective volume (default 32^3, R16G16B16A16). rgb = in-scattered luminance,
        //! a = opacity. z maps to distance with AerialPerspectiveDepthToSlice()/sqrt() (see
        //! SkyRayMarching.azsl for the exact lookup).
        SkyVolume,

        //! 32x32 multiple-scattering approximation LUT.
        MultiScattering
    };

    class SkyAtmosphereFeatureProcessorInterface
        : public AZ::RPI::FeatureProcessor
    {
        public:
        AZ_RTTI(SkyAtmosphereFeatureProcessorInterface, "{00C9FD3D-2A1B-49EA-97E3-952EF6C1C451}"); 

        using AtmosphereId = AZ::RHI::Handle<uint16_t, SkyAtmosphereFeatureProcessorInterface>;

        virtual AtmosphereId CreateAtmosphere() = 0;
        virtual void ReleaseAtmosphere(AtmosphereId id) = 0;
        virtual void SetAtmosphereParams(AtmosphereId id, const SkyAtmosphereParams& params) = 0;
        virtual void SetAtmosphereEnabled(AtmosphereId id, bool enabled) = 0;
        virtual bool GetAtmosphereEnabled(AtmosphereId id) = 0;

        //! Returns the image backing a precomputed LUT so other passes can sample the exact same
        //! atmosphere the sky is rendered with. Returns nullptr when the atmosphere has not built
        //! the image yet (the first frame(s)) or when no atmosphere is active on the pipeline.
        //! Pass AtmosphereId::Null to target the first atmosphere created on this feature processor.
        //! @param pipeline the render pipeline whose SkyAtmosphere pass owns the image; LUT images
        //!                 are per-pipeline, so a null pipeline returns nullptr.
        virtual AZ::Data::Instance<AZ::RPI::Image> GetLUTImage(
            AZ::RPI::RenderPipeline* pipeline, AtmosphereLUTType type,
            AtmosphereId id = AtmosphereId::Null) const = 0;

        //! Reads back the parameters an atmosphere is configured with. All lengths are kilometres
        //! (m_planetOrigin is the planet centre relative to world origin, in kilometres).
        //! Pass AtmosphereId::Null to target the first atmosphere created.
        virtual bool GetAtmosphereParams(
            AtmosphereId id, SkyAtmosphereParams& outParams) = 0;
    };
} // namespace SkyAtmosphere

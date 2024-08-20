// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <OpenImageDenoise/oidn.hpp>

#include "../openpgl_common.h"

namespace openpgl
{

struct Denoiser
{
    Denoiser(pgl_point2i resolution, bool filterFeatures) : m_resolution(resolution), m_filterFeatures(filterFeatures)
    {
        oidnDevice = oidn::newDevice();
        oidnDevice.commit();

        std::size_t numPixels = m_resolution.x * m_resolution.y;

        bufferColor3f = oidnDevice.newBuffer(numPixels * 3 * sizeof(float));
        bufferScalar = oidnDevice.newBuffer(numPixels * 1 * sizeof(float));
        bufferAlbedo = oidnDevice.newBuffer(numPixels * 3 * sizeof(float));
        bufferNormal = oidnDevice.newBuffer(numPixels * 3 * sizeof(float));

        if (filterFeatures)
        {
            oidnAlbedoFilter = oidnDevice.newFilter("RT");
            oidnAlbedoFilter.setImage("albedo", bufferAlbedo, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnAlbedoFilter.setImage("output", bufferAlbedo, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnAlbedoFilter.commit();

            oidnNormalFilter = oidnDevice.newFilter("RT");
            oidnNormalFilter.setImage("normal", bufferNormal, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnNormalFilter.setImage("output", bufferNormal, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnNormalFilter.commit();

            oidnColor3fFilter = oidnDevice.newFilter("RT");
            oidnColor3fFilter.setImage("color", bufferColor3f, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnColor3fFilter.setImage("albedo", bufferAlbedo, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnColor3fFilter.setImage("normal", bufferNormal, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnColor3fFilter.setImage("output", bufferColor3f, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnColor3fFilter.set("cleanAux", true);  // auxiliary images will be prefiltered
            oidnColor3fFilter.set("hdr", true);
            oidnColor3fFilter.commit();
        }
        else
        {
            oidnColor3fFilter = oidnDevice.newFilter("RT");
            oidnColor3fFilter.setImage("color", bufferColor3f, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnColor3fFilter.setImage("albedo", bufferAlbedo, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnColor3fFilter.setImage("normal", bufferNormal, oidn::Format::Float3, m_resolution.x, m_resolution.y);
            oidnColor3fFilter.setImage("output", bufferColor3f, oidn::Format::Float3, m_resolution.x, m_resolution.y);

            oidnColor3fFilter.set("hdr", true);
            oidnColor3fFilter.commit();
        }

        oidnScalarFilter = oidnDevice.newFilter("RT");
        oidnScalarFilter.setImage("color", bufferScalar, oidn::Format::Float, m_resolution.x, m_resolution.y);
        oidnScalarFilter.setImage("output", bufferScalar, oidn::Format::Float, m_resolution.x, m_resolution.y);
        oidnScalarFilter.set("hdr", true);
        oidnScalarFilter.commit();
    }

    void denoise(pgl_vec3f *rgb, pgl_vec3f *n, pgl_vec3f *albedo, pgl_vec3f *result)
    {
        const std::size_t numPixels = m_resolution.x * m_resolution.y;
        // Copy data to OIDN buffers
        bufferColor3f.write(0, numPixels * 3 * sizeof(float), rgb);
        bufferNormal.write(0, numPixels * 3 * sizeof(float), n);
        bufferAlbedo.write(0, numPixels * 3 * sizeof(float), albedo);
        if (m_filterFeatures)
        {
            oidnAlbedoFilter.execute();
            oidnNormalFilter.execute();
        }
        oidnColor3fFilter.execute();
        bufferColor3f.read(0, numPixels * 3 * sizeof(float), result);
    }

    void denoise(pgl_vec3f *rgb, pgl_vec3f *rgb2nd, pgl_vec3f *n, pgl_vec3f *albedo, pgl_vec3f *result, pgl_vec3f *result2nd)
    {
        const std::size_t numPixels = m_resolution.x * m_resolution.y;
        // Copy data to OIDN buffers
        bufferColor3f.write(0, numPixels * 3 * sizeof(float), rgb);
        bufferNormal.write(0, numPixels * 3 * sizeof(float), n);
        bufferAlbedo.write(0, numPixels * 3 * sizeof(float), albedo);
        if (m_filterFeatures)
        {
            oidnAlbedoFilter.execute();
            oidnNormalFilter.execute();
        }
        oidnColor3fFilter.execute();
        bufferColor3f.read(0, numPixels * 3 * sizeof(float), result);

        bufferColor3f.write(0, numPixels * 3 * sizeof(float), rgb2nd);
        oidnColor3fFilter.execute();
        bufferColor3f.read(0, numPixels * 3 * sizeof(float), result2nd);
    }

    void denoise(float *l, float *result)
    {
        const std::size_t numPixels = m_resolution.x * m_resolution.y;
        bufferScalar.write(0, numPixels * 1 * sizeof(float), l);
        oidnScalarFilter.execute();
        bufferScalar.read(0, numPixels * 1 * sizeof(float), result);
    }

   private:
    pgl_point2i m_resolution;

    bool m_filterFeatures{false};
    oidn::DeviceRef oidnDevice;

    oidn::FilterRef oidnColor3fFilter;
    oidn::FilterRef oidnScalarFilter;
    oidn::FilterRef oidnAlbedoFilter;
    oidn::FilterRef oidnNormalFilter;

    oidn::BufferRef bufferColor3f;
    oidn::BufferRef bufferScalar;
    oidn::BufferRef bufferAlbedo;
    oidn::BufferRef bufferNormal;
};

}  // namespace openpgl
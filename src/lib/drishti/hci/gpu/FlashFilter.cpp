/*!
  @file   gpu/FlashFilter.cpp
  @author David Hirvonen
  @brief  Illuminate the scene with pulses of LCD light.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#define FLASH_FILTER_USE_DELAY 1
#define FLASH_FILTER_USE_NEW_NMS 1

#include "drishti/hci/gpu/FlashFilter.h"

#if FLASH_FILTER_USE_DELAY
#  include "drishti/graphics/fade.h"
#endif
#include "drishti/graphics/saturation.h"
#include "drishti/graphics/binomial.h"

#include "ogles_gpgpu/common/proc/hessian.h"
#include "ogles_gpgpu/common/proc/gauss_opt.h"
#include "ogles_gpgpu/common/proc/fifo.h"
#include "ogles_gpgpu/common/proc/fir3.h"

#if FLASH_FILTER_USE_NEW_NMS
#  include "drishti/graphics/nms2.h"
#  define NMS_PROC ogles_gpgpu::Nms2Proc
#else
#  include "ogles_gpgpu/common/proc/nms.h"
#  define NMS_PROC ogles_gpgpu::NmsProc
#endif

#include <opencv2/core.hpp>

#include <limits>

BEGIN_OGLES_GPGPU

class FlashFilter::Impl
{
public:
    Impl(FlashFilter::FilterKind kind)
    : smoothProc1(1)
    , smoothProc2(1)
    , fifoProc(3)
    , fir3Proc(false)
    , hessianProc1(2000.0f, false)
    , hessianProc2(4000.0f, false)
#if FLASH_FILTER_USE_DELAY
    , fadeProc(0.95)
#endif
    , saturationProc(1.0)
    {
        fir3Proc.setAlpha(10.f);
        fir3Proc.setBeta(0.f); // drop negative values
        
        nmsProc1.setThreshold(0.05); // single image nms
        nmsProc1.swizzle(1, 3); // in(2), out(3)
        
        nmsProc2.setThreshold(0.05); // difference image nms
        nmsProc2.swizzle(1, 3); // in(2), out(3)
        
        switch(kind)
        {
            case FlashFilter::kLaplacian :
                fir3Proc.setWeights({-0.25, 0.5, -0.25});
                break;
            case FlashFilter::kCenteredDifference :
                //fir3Proc.setWeights({+0.5, 0.0, -0.5}); // bright->dark
                fir3Proc.setWeights({-0.5, 0.0, +0.5}); // dark->bright
                break;
        }
        
        smoothProc1.add(&fifoProc);
        smoothProc1.add(&hessianProc1);
        hessianProc1.add(&nmsProc1);
        
        //  (newest) RGB....RGB....RGB (oldest)
        fifoProc.addWithDelay(&fir3Proc, 0, 0);
        fifoProc.addWithDelay(&fir3Proc, 1, 1);
        fifoProc.addWithDelay(&fir3Proc, 2, 2);
        
        fir3Proc.add(&saturationProc);
        saturationProc.add(&smoothProc2);
        smoothProc2.add(&hessianProc2);
#if FLASH_FILTER_USE_DELAY
        hessianProc2.add(&fadeProc);
#endif
        fadeProc.add(&fadeSmoothProc);
        fadeSmoothProc.add(&nmsProc2);
    }
    
    ogles_gpgpu::GaussOptProc smoothProc1;
    ogles_gpgpu::GaussOptProc smoothProc2;
    ogles_gpgpu::BinomialProc fadeSmoothProc;
    ogles_gpgpu::FIFOPRoc fifoProc;
    ogles_gpgpu::Fir3Proc fir3Proc;
    ogles_gpgpu::HessianProc hessianProc1; // single image hessian
    ogles_gpgpu::HessianProc hessianProc2; // difference image hessian
#if FLASH_FILTER_USE_DELAY
    ogles_gpgpu::FadeFilterProc fadeProc;
#endif
    NMS_PROC nmsProc1; // single image nms
    NMS_PROC nmsProc2; // difference image nms
    ogles_gpgpu::SaturationProc saturationProc;
};

FlashFilter::FlashFilter(FilterKind kind)
{
    m_impl = std::make_shared<Impl>(kind);
    
    // Add filters to procPasses for state management
    procPasses.push_back(&m_impl->smoothProc1);
    procPasses.push_back(&m_impl->smoothProc2);
    procPasses.push_back(&m_impl->fifoProc);
    procPasses.push_back(&m_impl->fir3Proc);
    procPasses.push_back(&m_impl->fadeProc);
    procPasses.push_back(&m_impl->nmsProc2);
    procPasses.push_back(&m_impl->saturationProc);
}

FlashFilter::~FlashFilter()
{
    procPasses.clear();
}

ProcInterface* FlashFilter::getInputFilter() const
{
    return &m_impl->smoothProc1;
}

ProcInterface* FlashFilter::getHessianOfSingleImage() const
{
    return &m_impl->hessianProc1;
}

ProcInterface* FlashFilter::getHessianOfDifferenceImage() const
{
    return &m_impl->hessianProc2;
}

ProcInterface* FlashFilter::getHessianPeaksFromSingleImage() const
{
    return &m_impl->nmsProc1;
}

ProcInterface* FlashFilter::getHessianPeaksFromDifferenceImage() const
{
    return &m_impl->nmsProc2;
}

ProcInterface* FlashFilter::getOutputFilter() const
{
#if FLASH_FILTER_USE_DELAY
    return &m_impl->nmsProc2;
#else
    return &m_impl->hessianProc2;
#endif
}

int FlashFilter::render(int position)
{
    // Execute internal filter chain
    getInputFilter()->process(position);
    return 0;
}

int FlashFilter::init(int inW, int inH, unsigned int order, bool prepareForExternalInput)
{
    getInputFilter()->prepare(inW, inH, 0, std::numeric_limits<int>::max(), 0);
    return 0;
}

int FlashFilter::reinit(int inW, int inH, bool prepareForExternalInput)
{
    getInputFilter()->prepare(inW, inH, 0, std::numeric_limits<int>::max(), 0);
    return 0;
}

static cv::Size ogles_gpgpu_size(const ogles_gpgpu::Size2d &src)
{
    return cv::Size(src.width, src.height);
}

cv::Mat FlashFilter::paint()
{
    cv::Mat canvas;
    
    if(m_impl->fifoProc.getBufferCount() == 3)
    {
        cv::Mat4b smoothProc1(ogles_gpgpu_size(m_impl->smoothProc1.getOutFrameSize()));
        cv::Mat4b fifoProc0(ogles_gpgpu_size(m_impl->fifoProc.getOutFrameSize()));
        cv::Mat4b fifoProc1(ogles_gpgpu_size(m_impl->fifoProc.getOutFrameSize()));
        cv::Mat4b fifoProc2(ogles_gpgpu_size(m_impl->fifoProc.getOutFrameSize()));
        cv::Mat4b fir3Proc(ogles_gpgpu_size(m_impl->fir3Proc.getOutFrameSize()));
        cv::Mat4b smoothProc2(ogles_gpgpu_size(m_impl->smoothProc2.getOutFrameSize()));
        cv::Mat4b hessianProc1(ogles_gpgpu_size(m_impl->hessianProc1.getOutFrameSize()));
        cv::Mat4b hessianProc2(ogles_gpgpu_size(m_impl->hessianProc2.getOutFrameSize()));
        
        m_impl->smoothProc1.getResultData(smoothProc1.ptr());
        m_impl->fifoProc[0]->getResultData(fifoProc0.ptr());
        m_impl->fifoProc[1]->getResultData(fifoProc1.ptr());
        m_impl->fifoProc[2]->getResultData(fifoProc2.ptr());
        m_impl->fir3Proc.getResultData(fir3Proc.ptr());
        m_impl->smoothProc2.getResultData(smoothProc2.ptr());
        m_impl->hessianProc1.getResultData(hessianProc1.ptr());
        m_impl->hessianProc2.getResultData(hessianProc2.ptr());
        
        std::vector<cv::Mat> all
        {
            smoothProc1,
            fifoProc0,
            fifoProc1,
            fifoProc2,
            fir3Proc,
            smoothProc2,
            hessianProc1,
            hessianProc2
        };
        
#if FLASH_FILTER_USE_DELAY
        cv::Mat4b fadeProc(ogles_gpgpu_size(m_impl->fadeProc.getOutFrameSize()));
        m_impl->fadeProc.getResultData(fadeProc.ptr());
        all.emplace_back(fadeProc);
#endif
        
        cv::vconcat(all, canvas);
    }
    
    return canvas;
}

END_OGLES_GPGPU

/*!
  @file   finder/gpu/MultiTransformProc.cpp
  @author David Hirvonen (dhirvonen elucideye com)
  @brief Simple interface to apply parametric transformations to multiple image ROI's.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#include "face/gpu/MultiTransformProc.h"

#include "geometry/motion.h"

BEGIN_OGLES_GPGPU

void MultiTransformProc::renderRegion(const Rect2d &dstRoiPix, const Mat44f &H)
{
    // Limit warping to desired eye crop region
    //cv::Matx44f MVPt;
    //transformation::R3x3To4x4(H.t(), MVPt);
    glScissor(dstRoiPix.x, dstRoiPix.y, dstRoiPix.width, dstRoiPix.height);

    //cv::Vec3f c(rand()%1000, rand()%1000, rand()%1000); c = c * (1.0 / 1000.0f);
    //glClearColor(c[0], c[1], c[2], 1.f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 1
    glUniformMatrix4fv(shParamUTransform, 1, 0, (GLfloat *)&H.data[0]);
    Tools::checkGLErr(getProcName(), "MultiTransformProc::renderEye() : glUniformMatrix4fv()");

    glDrawArrays(GL_TRIANGLE_STRIP, 0, OGLES_GPGPU_QUAD_VERTICES);
    Tools::checkGLErr(getProcName(), "MultiTransformProc::renderEye() : glDrawArrays()");
#endif
}

void MultiTransformProc::filterRenderDraw()
{
    // TODO: override the main set coords method:
    cv::Rect roi(0,0,inFrameW, inFrameH);
    cv::Point2f tl = roi.tl(), br = roi.br(), tr(br.x, tl.y), bl(tl.x, br.y);
    std::vector<cv::Point2f> vertices { tl, tr, bl, br };

    glEnableVertexAttribArray(shParamAPos);
    glVertexAttribPointer(shParamAPos, 2, GL_FLOAT, GL_FALSE, 0, & vertices[0]);
    Tools::checkGLErr(getProcName(), "FacePainter::renderFaces() : glVertexAttribPointer()");

    glEnable(GL_SCISSOR_TEST);
    for(int i = 0; i < m_crops.size(); i++)
    {
        renderRegion(m_crops[i].roi, m_crops[i].H);
        Tools::checkGLErr(getProcName(), "render draw");
    }
    glDisable(GL_SCISSOR_TEST);

    m_crops.clear();
}

END_OGLES_GPGPU

/**
 *  @file   larpandora/LArPandoraInterface/LArPandoraTransformationPlugin.h
 *
 *  @brief  Header file for the LArPandora transformation plugin class.
 *
 *  $Log: $
 */
#ifndef LAR_PANDORA_TRANSFORMATION_PLUGIN_H
#define LAR_PANDORA_TRANSFORMATION_PLUGIN_H 1

#include "LArPlugins/LArTransformationPlugin.h"

namespace lar_pandora
{

/**
 *  @brief  LArPandoraTransformationPlugin class
 */
class LArPandoraTransformationPlugin : public lar_content::LArTransformationPlugin
{
public:

    /**
     *  @brief  Constructor
     *
     *  @param  thetaU  the angle from the W axis to the U axis (radians)
     *  @param  thetaV  the angle from the W axis to the V axis (radians)
     *  @param  sigmaUVW  nominal spatial resolution for U, V and W (cm)
     *  @param  wireZPitch  the wire pitch (cm)
     */
    LArPandoraTransformationPlugin(const float thetaU, const float thetaV, const float sigmaUVW, const float wireZPitch);

    /**
     *  @brief  Destructor
     */
    virtual ~LArPandoraTransformationPlugin();

    virtual float UVtoW(const float u, const float v) const;

    virtual float VWtoU(const float v, const float w) const;

    virtual float WUtoV(const float w, const float u) const;

    virtual float UVtoY(const float u, const float v) const;

    virtual float UVtoZ(const float u, const float v) const;

    virtual float YZtoU(const float y, const float z) const;

    virtual float YZtoV(const float y, const float z) const;

    virtual float GetSigmaUVW() const;

    virtual void GetMinChiSquaredYZ(const float u, const float v, const float w, const float sigmaU, const float sigmaV, const float sigmaW,
        float &y, float &z, float &chiSquared) const;

    virtual void GetProjectedYZ(const PositionAndType &hitPositionAndType, const PositionAndType &fitPositionAndType1,
        const PositionAndType &fitPositionAndType2, const float sigmaHit, const float sigmaFit, float &y, float &z, float &chiSquared) const;

private:

    const float     m_thetaU;          ///< inclination of U wires (radians)
    const float     m_thetaV;          ///< inclination of V wires (radians)
    const float     m_sigmaUVW;        ///< resolution (cm), for calculation of chi2
    const float     m_sinUminusV;      ///< sin(thetaU-thetaV)
    const float     m_sinUplusV;       ///< sin(thetaU+thetaV)
    const float     m_sinU;            ///< sin(thetaU)
    const float     m_sinV;            ///< sin(thetaV)
    const float     m_cosU;            ///< cos(thetaU)
    const float     m_cosV;            ///< cos(thetaV)
};

} // namespace lar_pandora

#endif // #ifndef LAR_PANDORA_TRANSFORMATION_PLUGIN_H
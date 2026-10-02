//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qFacets2                          #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: Ioannis Farmakis                #
//#                                                                        #
//##########################################################################

#include "qFacets2Process.h"

//local
#include "qFacets2Dialog.h"

//qCC_plugins
#include <ccMainAppInterface.h>

//qCC_pcp
#include <Partition.h>

using namespace PCP;

//! Default name for Facets2 scalar fields
static const char FACET_SF_NAME[] = "Facet ID";


bool qFacets2Process::Compute(const qFacets2Dialog& dlg,
                                QString& errorMessage,
                                ccPointCloud*& outputCloud,
                                ccHObject*& outputGroup,
                                bool allowDialogs,
                                QWidget* parentWidget,
                                ccMainAppInterface* app)
{
    //get the selected entities
    const ccHObject::Container& selectedEntities = app->getSelectedEntities();

    //check if there is at least one point cloud in the selection
    ccHObject::Container clouds;
    for ( ccHObject* entity : selectedEntities )
    {
        if ( entity && entity->isKindOf( CC_TYPES::POINT_CLOUD ) )
        {
            clouds.push_back( entity );
        }
    }

    if ( clouds.empty() )
    {
        errorMessage = "No point cloud has been selected!";
        return false;
    }

    // get the parameters from the dialog
    double resolution = dlg.getResolution();
    double minPlanarity = dlg.getMinPlanarity();
    float regularization = dlg.getRegularization();
    int32_t cutoff = dlg.getCutoff();
    // more parameteres (hard-coded for now)
    int32_t knn = 26;
    float spatialWeight = 0.00001f;
    

    for (ccGenericPointCloud* cloud : clouds)
	{
    if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD))
    {
        ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);

        // Remove any existing Facet ID scalar field before processing,
        // so it is not counted in D and does not contaminate Y data
        {
            int prevSfIdx = pc->getScalarFieldIndexByName(FACET_SF_NAME);
            if (prevSfIdx >= 0)
            {
                pc->deleteScalarField(prevSfIdx);
            }
        }

        ccOctree::Shared theOctree = cloud->getOctree();
        if (!theOctree)
        {
            ccProgressDialog pOctreeDlg(true, this);
            theOctree = cloud->computeOctree(&pOctreeDlg);
            if (!theOctree)
            {
                ccConsole::Error(tr("Couldn't compute octree for cloud '%1'!").arg(cloud->getName()));
                break;
            }
        }

        // we create/activate Cut Pursuit's label scalar field
        int sfIdx = pc->getScalarFieldIndexByName(CC_CUT_PURSUIT_LABEL_NAME);
        if (sfIdx < 0)
        {
            sfIdx = pc->addScalarField(CC_CUT_PURSUIT_LABEL_NAME);
        }
        if (sfIdx < 0)
        {
            ccConsole::Error(tr("Couldn't allocate a new scalar field for computing Cut Pursuit labels! Try to free some memory ..."));
            break;
        }
        pc->setCurrentScalarField(sfIdx);

        // determine which scalar fields to include in Y, based on the user's selection
        // (always excluding the Cut Pursuit label field itself)
        std::vector<unsigned> sfIndices;
        for (unsigned j = 0; j < pc->getNumberOfScalarFields(); ++j)
        {
            if (static_cast<int>(j) == sfIdx)
                continue;

            QString sfName = QString::fromStdString(pc->getScalarFieldName(j));
            if (sfName == CC_CUT_PURSUIT_LABEL_NAME)
                continue;

            if (s_selectedSFNames.contains(sfName))
                sfIndices.push_back(j);
        }

        // some parallel cut pursuit params
        size_t             rgbDim = (s_useRGB && pc->hasColors()) ? 3 : 0;
        int32_t            D      = 3 + static_cast<int32_t>(sfIndices.size()) + static_cast<int32_t>(rgbDim);
        int32_t            N      = static_cast<int32_t>(pc->size());
        std::vector<float> Y(N * D, 0.0f);

        CCVector3 posOffset(0, 0, 0);
        for (int32_t i = 0; i < N; ++i)
        {
            posOffset += *pc->getPoint(i);
        }
        posOffset /= static_cast<float>(N);

        for (int32_t i = 0; i < N; ++i)
        {
            const CCVector3* P = pc->getPoint(i);

            Y[i * D + 0] = static_cast<float>(P->x - posOffset.x);
            Y[i * D + 1] = static_cast<float>(P->y - posOffset.y);
            Y[i * D + 2] = static_cast<float>(P->z - posOffset.z);

            if (s_useRGB && pc->hasColors())
            {
                const ccColor::Rgba& C = pc->getPointColor(i);
                Y[i * D + 3]           = static_cast<float>(C.r / 255.0);
                Y[i * D + 4]           = static_cast<float>(C.g / 255.0);
                Y[i * D + 5]           = static_cast<float>(C.b / 255.0);
            }

            for (size_t k = 0; k < sfIndices.size(); ++k)
            {
                ccScalarField::Shared sf    = std::static_pointer_cast<ccScalarField>(pc->getScalarField(sfIndices[k]));
                float                 value = static_cast<float>(sf->getValue(i));

                // Sanitize NaN/Inf, force it to 0.0
                if (std::isnan(value) || std::isinf(value))
                {
                    value = 0.0f;
                }
                // Scalar fields start at feature index 3, if RGB is used, they start at feature index 6
                Y[i * D + 3 + rgbDim + k] = value;
            }
        }

        // we try to label all CCs
        std::vector<int32_t> components;
        int                  rV = PCP::Partition::labelCutPursuitComponents(cloud,
                                                            knn,
                                                            resolution, // knnRadius
                                                            N,
                                                            D,
                                                            Y,
                                                            regularization,
                                                            spatialWeight,
                                                            cutoff,
                                                            components,
                                                            &pDlg,
                                                            theOctree.data());

        // error handling
        if (rV < 0)
        {
            ccConsole::Error(tr("[Cut Pursuit] Failed to compute components!"));
            return;
        }

        // Assign component index to each point
        ccScalarField::Shared sf = std::static_pointer_cast<ccScalarField>(pc->getScalarField(sfIdx));
        for (int32_t i = 0; i < N; ++i)
        {
            sf->setValue(i, static_cast<ScalarType>(components[i]));
        }
        sf->computeMinAndMax();
    }

    return true;
}
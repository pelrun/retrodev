// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Raster document -- editor for raster effect code generation build items.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <generators/raster/raster.params.h>
#include <string>

namespace RetrodevGui {
	class DocumentRasterCpc;  // Forward declaration

	class DocumentRaster : public DocumentView {
	public:
		DocumentRaster(const std::string& name);
		~DocumentRaster() override;
		//
		// Render the raster document content
		//
		void Perform() override;
		//
		// Save the raster document (serializes library state back to project)
		//
		bool Save() override;
		//
		// Get the build type this document represents
		//
		RetrodevLib::ProjectBuildType GetBuildType() const override { return RetrodevLib::ProjectBuildType::Raster; }

	private:
		DocumentRasterCpc* m_cpcPanel = nullptr;  // Non-owning pointer to the active CPC panel
	};

}

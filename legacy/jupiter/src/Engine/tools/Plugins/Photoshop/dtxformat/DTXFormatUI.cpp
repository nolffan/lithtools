// ADOBE SYSTEMS INCORPORATED
// Copyright  1993 - 2002 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this 
// file in accordance with the terms of the Adobe license agreement
// accompanying it.  If you have received this file from a source
// other than Adobe, then your use, modification, or distribution
// of it requires the prior written permission of Adobe.
//-------------------------------------------------------------------
/*
	File: DTXFormatUI.cpp

*/

#include "PhotoshopSDK.h"
#include "DTXFormat.h"
#include "TextureProp.h"

namespace 
{
	enum BPPIdent
	{
		BPP_8P=0,		// 8 bit palettized
		BPP_8,			// 8 bit RGB
		BPP_16,
		BPP_32,
		BPP_S3TC_DXT1,
		BPP_S3TC_DXT3,
		BPP_S3TC_DXT5,
		BPP_32P,		//! this was added for true color pallete support
		NUM_BIT_TYPES
	};


const int16 kDOK					= 1;
const int16 kDCancel				= 2;

const int16 kDFlags					= 3;
const int16 kDGroup					= 4;
const int16 kDMipMaps				= 5;
const int16 kDNonS3TCMipmapOffset	= 6;
const int16 kDCommandString			= 7;
const int16 kDUIMipmapOffset		= 8;
const int16 kDDetailTextureScale	= 9;
const int16 kDDetailTextureAngle	= 10;
const int16 kD32Bit					= 11;
const int16 kDDXT1					= 12;
const int16 kDDXT3					= 13;
const int16 kDDXT5					= 14;
const int16 kD4444					= 15;
const int16 kD5551					= 16;
const int16 kDPrefer16Bit			= 17;
const int16 kDFullBrights			= 18;
const int16 kDPreserve32Bit			= 19;
const int16 kDNoSysCache			= 20;

} //end namespace

class DTXFormatDialog : public PIDialog 
{
private:

	// Info
	PIText textFlags;
	PIText textGroup;
	PIText textMipmaps;
	PIText textNonS3TCMipmapOffset;
	PIText textCommandString;
	PIText textUIMipmapOffset;
	PIText textDetailTextureScale;
	PIText textDetailTextureAngle;

	// Compression
	PICheckBox checkbox32Bit;
	PICheckBox checkboxDXT1;
	PICheckBox checkboxDXT3;
	PICheckBox checkboxDXT5;

	// Texture format
	PICheckBox checkbox4444;
	PICheckBox checkbox5551;

	// Misc
	PICheckBox checkboxPrefer16Bit;
	PICheckBox checkboxFullBrights;
	PICheckBox checkboxPreserve32Bit;
	PICheckBox checkboxNoSysCache;

	virtual void Init(void);
	virtual void Notify(int32 item);

	void	FillDialogItems();
	TextureProp *m_TextureProp;

public:
	DTXFormatDialog(TextureProp* pProp) : PIDialog(),
				checkbox32Bit(),
				checkboxDXT1(),
				checkboxDXT3(),
				checkboxDXT5(),
				checkbox4444(),
				checkbox5551(),
				checkboxPrefer16Bit(),
				checkboxFullBrights(),
				checkboxPreserve32Bit(),
				checkboxNoSysCache(),
				textFlags(),
				textGroup(),
				textMipmaps(),
				textNonS3TCMipmapOffset(),
				textCommandString(),
				textUIMipmapOffset(),
				textDetailTextureScale(),
				textDetailTextureAngle(),
				m_TextureProp(NULL)
	{
		m_TextureProp = pProp;
	}

	~DTXFormatDialog() 
	{
		m_TextureProp = NULL;
	}

	void	ApplyDialogItems();
};

bool DoUI (TextureProp* pProp)
{
	DTXFormatDialog dialog(pProp);

	int result = dialog.Modal(gPluginRef, NULL, 16050);
	if (result == kDOK)
	{
	}
	else
	{
		return false;
	}

	return (result == kDOK);
}

void DTXFormatDialog::Init(void)
{
	PIItem item;
	PIDialogPtr dialog = GetDialog();
	
	//
	// INFO
	//
	item = PIGetDialogItem(dialog, kDFlags);
	textFlags.SetItem(item);

	item = PIGetDialogItem(dialog, kDGroup);
	textGroup.SetItem(item);

	item = PIGetDialogItem(dialog, kDMipMaps);
	textMipmaps.SetItem(item);

	item = PIGetDialogItem(dialog, kDNonS3TCMipmapOffset);
	textNonS3TCMipmapOffset.SetItem(item);

	item = PIGetDialogItem(dialog, kDCommandString);
	textCommandString.SetItem(item);

	item = PIGetDialogItem(dialog, kDUIMipmapOffset);
	textUIMipmapOffset.SetItem(item);

	item = PIGetDialogItem(dialog, kDDetailTextureScale);
	textDetailTextureScale.SetItem(item);

	item = PIGetDialogItem(dialog, kDDetailTextureAngle);
	textDetailTextureAngle.SetItem(item);
			
	//
	// COMPRESSION
	//
	item = PIGetDialogItem(dialog, kD32Bit);
	checkbox32Bit.SetItem(item);
	checkbox32Bit.SetChecked(false);

	item = PIGetDialogItem(dialog, kDDXT1);
	checkboxDXT1.SetItem(item);
	checkboxDXT1.SetChecked(false);

	item = PIGetDialogItem(dialog, kDDXT3);
	checkboxDXT3.SetItem(item);
	checkboxDXT3.SetChecked(false);

	item = PIGetDialogItem(dialog, kDDXT5);
	checkboxDXT5.SetItem(item);
	checkboxDXT5.SetChecked(false);

	//
	// TEXTURE FORMAT
	//
	item = PIGetDialogItem(dialog, kD4444);
	checkbox4444.SetItem(item);
	checkbox4444.SetChecked(false);

	item = PIGetDialogItem(dialog, kD5551);
	checkbox5551.SetItem(item);
	checkbox5551.SetChecked(false);

	//
	// MISC
	//
	item = PIGetDialogItem(dialog, kDPrefer16Bit);
	checkboxPrefer16Bit.SetItem(item);

	item = PIGetDialogItem(dialog, kDFullBrights);
	checkboxFullBrights.SetItem(item);

	item = PIGetDialogItem(dialog, kDPreserve32Bit);
	checkboxPreserve32Bit.SetItem(item);

	item = PIGetDialogItem(dialog, kDNoSysCache);
	checkboxNoSysCache.SetItem(item);

	// Ok, now fill the values
	FillDialogItems();	
}

void DTXFormatDialog::Notify(int32 item)
{
	switch(item)
	{
	case kD32Bit:
		{
			checkbox32Bit.SetChecked(true);
			checkboxDXT1.SetChecked(false);
			checkboxDXT3.SetChecked(false);
			checkboxDXT5.SetChecked(false);
		}
		break;
	case kDDXT1:
		{
			checkbox32Bit.SetChecked(false);
			checkboxDXT1.SetChecked(true);
			checkboxDXT3.SetChecked(false);
			checkboxDXT5.SetChecked(false);
		}
		break;
	case kDDXT3:
		{
			checkbox32Bit.SetChecked(false);
			checkboxDXT1.SetChecked(false);
			checkboxDXT3.SetChecked(true);
			checkboxDXT5.SetChecked(false);
		}
		break;
	case kDDXT5:
		{
			checkbox32Bit.SetChecked(false);
			checkboxDXT1.SetChecked(false);
			checkboxDXT3.SetChecked(false);
			checkboxDXT5.SetChecked(true);
		}
		break;
	case kD4444:
		{
			checkbox5551.SetChecked(false);
		}
		break;
	case kD5551:
		{
			checkbox4444.SetChecked(false);
		}
		break;
	case kDOK:
		{
			ApplyDialogItems();
		}break;
	default:
		break;
	}
}

void DTXFormatDialog::FillDialogItems()
{
	if(NULL == m_TextureProp)
	{
		return;
	}

	// Make sure our members vars are up to date.
	m_TextureProp->FillMembers();

	char szBuf[256];

	// Info
	sprintf(szBuf, "%d", m_TextureProp->m_TextureFlags);
	textFlags.SetText(szBuf);

	sprintf(szBuf, "%d", m_TextureProp->m_TextureGroup);
	textGroup.SetText(szBuf);

	sprintf(szBuf, "%d", m_TextureProp->m_nMipmaps);
	textMipmaps.SetText(szBuf);

	sprintf(szBuf, "%d", m_TextureProp->m_NonS3TCMipmapOffset);
	textNonS3TCMipmapOffset.SetText(szBuf);

	sprintf(szBuf, "%s", m_TextureProp->m_CommandString);
	textCommandString.SetText(szBuf);

	sprintf(szBuf, "%d", m_TextureProp->m_UIMipmapOffset);
	textUIMipmapOffset.SetText(szBuf);

	sprintf(szBuf, "%f", m_TextureProp->m_DetailTextureScale);
	textDetailTextureScale.SetText(szBuf);

	sprintf(szBuf, "%d", m_TextureProp->m_DetailTextureAngle);
	textDetailTextureAngle.SetText(szBuf);

	// Compression
	switch(m_TextureProp->m_BPPIdent)
	{
	case BPP_32:
		{
			checkbox32Bit.SetChecked(true);
			checkboxDXT1.SetChecked(false);
			checkboxDXT3.SetChecked(false);
			checkboxDXT5.SetChecked(false);
		}
		break;
	case BPP_S3TC_DXT1:
		{
			checkbox32Bit.SetChecked(false);
			checkboxDXT1.SetChecked(true);
			checkboxDXT3.SetChecked(false);
			checkboxDXT5.SetChecked(false);
		}
		break;
	case BPP_S3TC_DXT3:
		{
			checkbox32Bit.SetChecked(false);
			checkboxDXT1.SetChecked(false);
			checkboxDXT3.SetChecked(true);
			checkboxDXT5.SetChecked(false);
		}
		break;
	case BPP_S3TC_DXT5:
		{
			checkbox32Bit.SetChecked(false);
			checkboxDXT1.SetChecked(false);
			checkboxDXT3.SetChecked(false);
			checkboxDXT5.SetChecked(true);
		}
		break;
	default:
		{
			checkbox32Bit.SetChecked(true);
			checkboxDXT1.SetChecked(false);
			checkboxDXT3.SetChecked(false);
			checkboxDXT5.SetChecked(false);
		}
		break;
	}

	//Texture properties
	if(m_TextureProp->m_bPrefer4444)
	{
		checkbox4444.SetChecked(true);
		checkbox5551.SetChecked(false);
	}

	if(m_TextureProp->m_bPrefer5551)
	{
		checkbox4444.SetChecked(false);
		checkbox5551.SetChecked(true);
	}

	//Misc
	if(m_TextureProp->m_bPrefer16Bit)
	{
		checkboxPrefer16Bit.SetChecked(true);
	}

	if(m_TextureProp->m_bFullBrights)
	{
		checkboxFullBrights.SetChecked(true);
	}

	if(m_TextureProp->m_b32BitSysCopy)
	{
		checkboxPreserve32Bit.SetChecked(true);
	}

	if(m_TextureProp->m_bNoSysCache)
	{
		checkboxNoSysCache.SetChecked(true);
	}

}

void DTXFormatDialog::ApplyDialogItems()
{
	if(NULL == m_TextureProp)
	{
		return;
	}

	//Set the info from our dialog

	// Info
	char* sText = textFlags.GetText();
	if(NULL != sText){m_TextureProp->m_TextureFlags = (int)atoi(sText);}

	sText = textGroup.GetText();
	if(NULL != sText){m_TextureProp->m_TextureGroup = (int)atoi(sText);}

	sText = textMipmaps.GetText();
	if(NULL != sText){m_TextureProp->m_nMipmaps = (int)atoi(sText);}

	sText = textNonS3TCMipmapOffset.GetText();
	if(NULL != sText){m_TextureProp->m_NonS3TCMipmapOffset = (int)atoi(sText);}

	sText = textCommandString.GetText();
	if(NULL != sText){strncpy(m_TextureProp->m_CommandString, sText, 127);}

	sText = textUIMipmapOffset.GetText();
	if(NULL != sText){m_TextureProp->m_UIMipmapOffset = (int)atoi(sText);}

	sText = textDetailTextureScale.GetText();
	if(NULL != sText){m_TextureProp->m_DetailTextureScale = (float)atof(sText);}

	sText = textDetailTextureAngle.GetText();
	if(NULL != sText){m_TextureProp->m_DetailTextureAngle = (int)atoi(sText);}

	// Compression
	if(checkbox32Bit.GetChecked()){m_TextureProp->m_TempBPPIdent = BPP_32;}
	else
	if(checkboxDXT1.GetChecked()){m_TextureProp->m_TempBPPIdent = BPP_S3TC_DXT1;}
	else
	if(checkboxDXT3.GetChecked()){m_TextureProp->m_TempBPPIdent = BPP_S3TC_DXT3;}
	else
	if(checkboxDXT5.GetChecked()){m_TextureProp->m_TempBPPIdent = BPP_S3TC_DXT5;}

	//Texture properties
	if(checkbox4444.GetChecked()){m_TextureProp->m_bPrefer4444 = TRUE;}
	else
	if(checkbox5551.GetChecked()){m_TextureProp->m_bPrefer5551 = TRUE;}

	//Misc
	if(checkboxPrefer16Bit.GetChecked()){m_TextureProp->m_bPrefer16Bit = TRUE;}
	if(checkboxFullBrights.GetChecked()){m_TextureProp->m_bFullBrights = TRUE;}
	if(checkboxPreserve32Bit.GetChecked()){m_TextureProp->m_b32BitSysCopy = TRUE;}
	if(checkboxNoSysCache.GetChecked()){m_TextureProp->m_bNoSysCache = TRUE;}

	m_TextureProp->StoreMembers();
}

// end DTXFormatUI.cpp
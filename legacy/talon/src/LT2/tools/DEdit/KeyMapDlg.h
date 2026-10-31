// KeyMapDlg.h - the 2001 Key Mappings dialog (Help, ID_KEYMAP), written from
// the image (0x42ce61..0x42d25c).  The names are ours (talon), the string
// ids resource.h's.

#ifndef __KEYMAPDLG_H__
#define __KEYMAPDLG_H__

	class CKeyMapDlg : public CDialog
	{
	public:
						CKeyMapDlg(CWnd* pParent = NULL);	// 0x42ce61

		enum { IDD = IDD_KEYMAP };

		CListCtrl		m_KeyList;		// 0x5c

	protected:
		virtual void	DoDataExchange(CDataExchange* pDX);
		virtual BOOL	OnInitDialog();

		DECLARE_MESSAGE_MAP()
	};

#endif

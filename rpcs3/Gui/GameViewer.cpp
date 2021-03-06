#include "stdafx.h"
#include "GameViewer.h"
#include "Loader/PSF.h"

static const std::string m_class_name = "GameViewer";
GameViewer::GameViewer(wxWindow* parent) : wxListView(parent)
{
	LoadSettings();
	m_columns.Show(this);

	m_path = "/dev_hdd0/game/";

	Bind(wxEVT_LIST_ITEM_ACTIVATED, &GameViewer::DClick, this);
	Bind(wxEVT_LIST_COL_CLICK, &GameViewer::OnColClick, this);

	Refresh();
}

GameViewer::~GameViewer()
{
	SaveSettings();
}

void GameViewer::DoResize(wxSize size)
{
	SetSize(size);
}

int wxCALLBACK ListItemCompare(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortinfo)
{
	ListSortInfo *SortInfo = (ListSortInfo *)sortinfo;
	int Column = SortInfo->Column;
	GameViewer *pGameViewer = SortInfo->GameViewerCtrl; 
	bool SortAscending = SortInfo->SortAscending; 
	long index1 = pGameViewer->FindItem(-1, item1);  
	long index2 = pGameViewer->FindItem(-1, item2);  
	wxString string1 = pGameViewer->GetItemText(index1, Column); 
	wxString string2 = pGameViewer->GetItemText(index2, Column); 

	if (string1.Cmp(string2) < 0)
	{
		return SortAscending ? -1 : 1;
	}
	else
		if (string1.Cmp(string2) > 0)
		{
		return SortAscending ? 1 : -1;
		}
		else
		{
			return 0;
		}
}

void GameViewer::OnColClick(wxListEvent& event)
{
	if (event.GetColumn() == SortInfo.Column)  
	{
		SortInfo.SortAscending = SortInfo.SortAscending ? FALSE : TRUE;
	}
	else 
	{
		SortInfo.SortAscending = TRUE;
	}
	SortInfo.Column = event.GetColumn();
	SortInfo.GameViewerCtrl = this;  
	SortItems(ListItemCompare, (long)&SortInfo);  

}


void GameViewer::LoadGames()
{
	vfsDir dir(m_path);
	ConLog.Write("path: %s", m_path.c_str());
	if(!dir.IsOpened()) return;

	m_games.clear();

	for(const DirEntryInfo* info = dir.Read(); info; info = dir.Read())
	{
		if(info->flags & DirEntry_TypeDir)
		{
			m_games.push_back(info->name);
		}
	}

	//ConLog.Write("path: %s", m_path.wx_str());
	//ConLog.Write("folders count: %d", m_games.GetCount());
}

void GameViewer::LoadPSF()
{
	m_game_data.clear();
	for(uint i=0; i<m_games.size(); ++i)
	{
		const std::string path = m_path + m_games[i] + "/PARAM.SFO";
		vfsFile f;
		if(!f.Open(path))
			continue;

		PSFLoader psf(f);
		if(!psf.Load(false))
			continue;

		GameInfo game;
		game.root = m_games[i];
		game.serial = psf.GetString("TITLE_ID");
		game.name = psf.GetString("TITLE");
		game.app_ver = psf.GetString("APP_VER");
		game.category = psf.GetString("CATEGORY");
		game.fw = psf.GetString("PS3_SYSTEM_VER");
		game.parental_lvl = psf.GetInteger("PARENTAL_LEVEL");
		game.resolution = psf.GetInteger("RESOLUTION");
		game.sound_format = psf.GetInteger("SOUND_FORMAT");
		if(game.serial.length() == 9)
			game.serial = game.serial.substr(0, 4) + "-" + game.serial.substr(4, 5);

		m_game_data.push_back(game);
	}

	m_columns.Update(m_game_data);
}

void GameViewer::ShowData()
{
	m_columns.ShowData(this);
}

void GameViewer::Refresh()
{
	Emu.GetVFS().Init(m_path);
	LoadGames();
	LoadPSF();
	ShowData();
	Emu.GetVFS().UnMountAll();
}

void GameViewer::SaveSettings()
{
	m_columns.LoadSave(false, m_class_name, this);
}

void GameViewer::LoadSettings()
{
	m_columns.LoadSave(true, m_class_name);
}

void GameViewer::DClick(wxListEvent& event)
{
	long i = GetFirstSelected();
	if(i < 0) return;

	const std::string& path = m_path + m_game_data[i].root;

	Emu.Stop();
	Emu.GetVFS().Init(path);
	std::string local_path;
	if(Emu.GetVFS().GetDevice(path, local_path) && !Emu.BootGame(local_path))
	{
		ConLog.Error("Boot error: elf not found! [%s]", path.c_str());
		return;
	}
	Emu.Run();
}

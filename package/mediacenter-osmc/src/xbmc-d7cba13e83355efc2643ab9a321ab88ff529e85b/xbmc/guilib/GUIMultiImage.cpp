/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIMultiImage.h"
#include "TextureManager.h"
#include "filesystem/Directory.h"
#include "utils/URIUtils.h"
#include "utils/JobManager.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "input/Key.h"
#include "TextureCache.h"
#include "WindowIDs.h"
#include "utils/StringUtils.h"

using namespace XFILE;

CGUIMultiImage::CGUIMultiImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, unsigned int timePerImage, unsigned int fadeTime, bool randomized, bool loop, unsigned int timeToPauseAtEnd)
    : CGUIControl(parentID, controlID, posX, posY, width, height),
      m_image(0, 0, posX, posY, width, height, texture)
{
  m_currentImage = 0;
  m_timePerImage = timePerImage + fadeTime;
  m_timeToPauseAtEnd = timeToPauseAtEnd;
  m_image.SetCrossFade(fadeTime);
  m_randomized = randomized;
  m_loop = loop;
  ControlType = GUICONTROL_MULTI_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_directoryStatus = UNLOADED;
  m_jobID = 0;
}

CGUIMultiImage::CGUIMultiImage(const CGUIMultiImage &from)
  : CGUIControl(from), m_texturePath(from.m_texturePath), m_imageTimer(), m_files(), m_image(from.m_image)
{
  m_timePerImage = from.m_timePerImage;
  m_timeToPauseAtEnd = from.m_timeToPauseAtEnd;
  m_randomized = from.m_randomized;
  m_loop = from.m_loop;
  m_bDynamicResourceAlloc=false;
  m_directoryStatus = UNLOADED;
  if (m_texturePath.IsConstant())
    m_currentPath = m_texturePath.GetLabel(WINDOW_INVALID);
  m_currentImage = 0;
  ControlType = GUICONTROL_MULTI_IMAGE;
  m_jobID = 0;
}

CGUIMultiImage::~CGUIMultiImage(void)
{
  CancelLoading();
}

void CGUIMultiImage::UpdateVisibility(const CGUIListItem *item)
{
  CGUIControl::UpdateVisibility(item);

  // check if we're hidden, and deallocate if so
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && m_bAllocated)
      FreeResources();
    return;
  }

  // we are either delayed or visible, so we can allocate our resources
  if (m_directoryStatus == UNLOADED)
    LoadDirectory();

  if (!m_bAllocated)
    AllocResources();

  if (m_directoryStatus == LOADED)
    OnDirectoryLoaded();
}

void CGUIMultiImage::UpdateInfo(const CGUIListItem *item)
{
  // check for conditional information before we
  // alloc as this can free our resources
  if (!m_texturePath.IsConstant())
  {
    std::string texturePath;
    if (item)
      texturePath = m_texturePath.GetItemLabel(item, true);
    else
      texturePath = m_texturePath.GetLabel(m_parentID);
    if (texturePath != m_currentPath && !texturePath.empty())
    {
      // a new path - set our current path and tell ourselves to load our directory
      m_currentPath = texturePath;
      CancelLoading();
    }
  }
}

void CGUIMultiImage::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // Set a viewport so that we don't render outside the defined area
  if (m_directoryStatus == READY && !m_files.empty())
  {
    unsigned int nextImage = m_currentImage + 1;
    if (nextImage >= m_files.size())
      nextImage = m_loop ? 0 : m_currentImage;  // stay on the last image if <loop>no</loop>

    if (nextImage != m_currentImage)
    {
      // check if we should be loading a new image yet
      unsigned int timeToShow = m_timePerImage;
      if (0 == nextImage) // last image should be paused for a bit longer if that's what the skinner wishes.
        timeToShow += m_timeToPauseAtEnd;
      if (m_imageTimer.IsRunning() && m_imageTimer.GetElapsedMilliseconds() > timeToShow)
      {
        // grab a new image
        m_currentImage = nextImage;
        m_image.SetFileName(m_files[m_currentImage]);
        MarkDirtyRegion();

        m_imageTimer.StartZero();
      }
    }
  }

  if (g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
  {
    if (m_image.SetColorDiffuse(m_diffuseColor))
      MarkDirtyRegion();

    m_image.DoProcess(currentTime, dirtyregions);

    g_graphicsContext.RestoreClipRegion();
  }

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIMultiImage::Render()
{
  m_image.Render();
  CGUIControl::Render();
}

bool CGUIMultiImage::OnAction(const CAction &action)
{
  return false;
}

bool CGUIMultiImage::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
  {
    if (!m_texturePath.IsConstant())
      FreeResources();
    return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIMultiImage::AllocResources()
{
  FreeResources();
  CGUIControl::AllocResources();

  if (m_directoryStatus == UNLOADED)
    LoadDirectory();
}

void CGUIMultiImage::FreeResources(bool immediately)
{
  m_image.FreeResources(immediately);
  m_currentImage = 0;
  CancelLoading();
  m_files.clear();
  CGUIControl::FreeResources(immediately);
}

void CGUIMultiImage::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_bDynamicResourceAlloc=bOnOff;
}

void CGUIMultiImage::SetInvalid()
{
  m_image.SetInvalid();
  CGUIControl::SetInvalid();
}

bool CGUIMultiImage::CanFocus() const
{
  return false;
}

void CGUIMultiImage::SetAspectRatio(const CAspectRatio &ratio)
{
  m_image.SetAspectRatio(ratio);
}

void CGUIMultiImage::LoadDirectory()
{
  // clear current stuff out
  m_files.clear();

  // don't load any images if our path is empty
  if (m_currentPath.empty()) return;

  /* Check the fast cases:
   1. Picture extension
   2. Cached picture (in case an extension is not present)
   3. Bundled folder
   */
  CFileItem item(m_currentPath, false);
  if (item.IsPicture() || CTextureCache::GetInstance().HasCachedImage(m_currentPath))
    m_files.push_back(m_currentPath);
  else // bundled folder?
    g_TextureManager.GetBundledTexturesFromPath(m_currentPath, m_files);
  if (!m_files.empty())
  { // found - nothing more to do
    OnDirectoryLoaded();
    return;
  }
  // slow(er) checks necessary - do them in the background
  CSingleLock lock(m_section);
  m_directoryStatus = LOADING;
  m_jobID = CJobManager::GetInstance().AddJob(new CMultiImageJob(m_currentPath), this, CJob::PRIORITY_NORMAL);
}

void CGUIMultiImage::OnDirectoryLoaded()
{
  // Randomize or sort our images if necessary
  if (m_randomized)
    random_shuffle(m_files.begin(), m_files.end());
  else
    sort(m_files.begin(), m_files.end());

  // flag as loaded - no point in constantly reloading them
  m_directoryStatus = READY;
  m_imageTimer.StartZero();
  m_currentImage = 0;
  m_image.SetFileName(m_files.empty() ? "" : m_files[0]);
}

void CGUIMultiImage::CancelLoading()
{
  CSingleLock lock(m_section);
  if (m_directoryStatus == LOADING)
    CJobManager::GetInstance().CancelJob(m_jobID);
  m_directoryStatus = UNLOADED;
}

void CGUIMultiImage::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CSingleLock lock(m_section);
  if (m_directoryStatus == LOADING && strncmp(job->GetType(), "multiimage", 10) == 0)
  {
    m_files = ((CMultiImageJob *)job)->m_files;
    m_directoryStatus = LOADED;
  }
}

void CGUIMultiImage::SetInfo(const CGUIInfoLabel &info)
{
  m_texturePath = info;
  if (m_texturePath.IsConstant())
    m_currentPath = m_texturePath.GetLabel(WINDOW_INVALID);
}

std::string CGUIMultiImage::GetDescription() const
{
  return m_image.GetDescription();
}

CGUIMultiImage::CMultiImageJob::CMultiImageJob(const std::string &path)
  : m_path(path)
{
}

bool CGUIMultiImage::CMultiImageJob::DoWork()
{
  // check to see if we have a single image or a folder of images
  CFileItem item(m_path, false);
  item.FillInMimeType();
  if (item.IsPicture() || StringUtils::StartsWithNoCase(item.GetMimeType(), "image/"))
  {
    m_files.push_back(m_path);
  }
  else
  {
    // Load in images from the directory specified
    // m_path is relative (as are all skin paths)
    std::string realPath = g_TextureManager.GetTexturePath(m_path, true);
    if (realPath.empty())
      return true;

    URIUtils::AddSlashAtEnd(realPath);
    CFileItemList items;
    CDirectory::GetDirectory(realPath, items, g_advancedSettings.m_pictureExtensions + "|.tbn|.dds", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_NO_FILE_INFO);
    for (int i=0; i < items.Size(); i++)
    {
      CFileItem* pItem = items[i].get();
      if (pItem && (pItem->IsPicture() || StringUtils::StartsWithNoCase(pItem->GetMimeType(), "image/")))
        m_files.push_back(pItem->GetPath());
    }
  }
  return true;
}

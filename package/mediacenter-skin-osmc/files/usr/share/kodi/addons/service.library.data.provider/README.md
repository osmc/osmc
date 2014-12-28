service.library.data.provider
============================

Python script for use with XBMC

============================

INFORMATION FOR SKINNERS
============================

Include the following in your addon.xml
`<import addon="service.library.data.provider" version="0.0.4"/>`

Load a list with this content tag to have the list use cached data automatically refresh:
`<content target="video">plugin://service.library.data.provider?type=randommovies&amp;reload=$INFO[Window.Property(randommovies)]</content>`

To load a list with multiple content types, split the types with a + and include the window property for each type:
`<content target="video">plugin://service.library.data.provider?type=randommovies+recentepisodes&amp;reload=$INFO[Window.Property(randommovies)]$INFO[Window.Property(recentepisodes)]</content>`

To view within the library, create a link omitting the reload parameter:
`<onclick>ActivateWindow(Videos,plugin://service.library.data.provider?type=randommovies,return)</onclick>`

Available tags:
-   randommovies
-   recentmovies
-   recommendedmovies
-   recommendedepisodes
-   recentepisodes
-   randomepisodes
-   recentvideos (movies and episodes)
-   randomsongs
-   randomalbums
-   recentalbums
-   recommendedalbums
-	playliststats

Available infolabels:

ListItem.Property(type) shows with what option the script was run.

Movies:
-	ListItem.Title
-	ListItem.OriginalTitle
-	ListItem.Year
-	ListItem.Genre
-	ListItem.Studio
-	ListItem.Country
-	ListItem.Plot
-	ListItem.PlotOutline
-	ListItem.Tagline
-	ListItem.Rating
-	ListItem.Votes
-	ListItem.MPAA
-	ListItem.Director
-	ListItem.Writer
-	ListItem.Cast
-	ListItem.CastAndRole
-	ListItem.Trailer
-	ListItem.Playcount
-	ListItem.Duration
-	ListItem.Property(resumetime)
-	ListItem.Property(totaltime)
-	ListItem.Property(dbid)
-	ListItem.Property(fanart_image)
-	ListItem.Icon `DefaultVideoCover.png`
-	ListItem.Thumb
-	ListItem.Art(poster)
-	ListItem.Art(thumb)
-	ListItem.Art(clearart)
-	ListItem.Art(clearlogo)
-	ListItem.Art(landscape)
-	ListItem.Art(fanart)
-	ListItem.VideoResolution
-	ListItem.VideoAspect
-	ListItem.AudioCodec
-	ListItem.AudioChannels
-	ListItem.AudioLanguage
-	ListItem.SubtitleLanguage

Episodes:
-	ListItem.Title
-	ListItem.Episode
-	ListItem.Season
- 	ListItem.Property(episodeno)
-	ListItem.Studio
-	ListItem.Premiered
-	ListItem.Plot
-	ListItem.TVshowTitle
-	ListItem.Rating
-	ListItem.MPAA
-	ListItem.Director
-	ListItem.Writer
-	ListItem.Cast
-	ListItem.CastAndRole
-	ListItem.Playcount
-	ListItem.Duration
-	ListItem.Property(resumetime)
-	ListItem.Property(totaltime)
-	ListItem.Property(dbid)
-	ListItem.Property(fanart_image)
-	ListItem.Icon `DefaultTVShows.png`
-	ListItem.Thumb
-	ListItem.Art(tvshow.poster)
-	ListItem.Art(thumb)
-	ListItem.Art(tvshow.clearart)
-	ListItem.Art(tvshow.clearlogo)
-	ListItem.Art(tvshow.landscape)
-	ListItem.Art(fanart)

Songs:
-	ListItem.Title
-	ListItem.Artist
-	ListItem.Genre
-	ListItem.Year
-	ListItem.Rating
-	ListItem.Album
-	ListItem.Icon `DefaultMusicSongs.png`
-	ListItem.Thumb
-	ListItem.Property(fanart_image)
-	ListItem.Property(dbid)

Albums:
-	ListItem.Title
-	ListItem.Artist
-	ListItem.Genre
-	ListItem.Year
-	ListItem.Rating
-	ListItem.Property(Album_Mood)
-	ListItem.Property(Album_Style)
-	ListItem.Property(Album_Theme)
-	ListItem.Property(Album_Type)
-	ListItem.Property(Album_Label)
-	ListItem.Property(Album_Description)
-	ListItem.Icon `DefaultAlbumCover.png`
-	ListItem.Thumb
-	ListItem.Property(fanart_image)
-	ListItem.Property(dbid)

Playliststats is used when a playlist or videonode is set as the onclick action in the (Home) menu.
Example:
Put a list in your Home.xml:
```xml
<control type="list" id="43260">
	<posx>0</posx>
	<posy>0</posy>
	<width>1</width>
	<height>1</height>
	<focusedlayout/>
	<itemlayout/>
	<content>plugin://service.library.data.provider?type=playliststats&amp;id=$INFO[Container(9000).ListItem.Property(Path)]</content>
</control>
```
The Path property has the onclick action defined. 
9000 is the ID of the Home main menu.
The following properties are available when the menu item containing the playlist or video node is highlighted:
-	Window(Home).Property(PlaylistWatched)
-	Window(Home).Property(PlaylistCount)
-	Window(Home).Property(PlaylistTVShowCount)
-	Window(Home).Property(PlaylistInProgress)
-	Window(Home).Property(PlaylistUnWatched)
-	Window(Home).Property(PlaylistEpisodes)
-	Window(Home).Property(PlaylistEpisodesUnWatched)




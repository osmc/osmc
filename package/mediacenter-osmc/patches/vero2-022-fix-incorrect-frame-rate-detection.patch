From 71cc76a1f8b1a0541cf3e2cf97a3e8653821134f Mon Sep 17 00:00:00 2001
From: Alex Deryskyba <alex@codesnake.com>
Date: Sun, 21 Sep 2014 17:20:25 +0300
Subject: [PATCH 6/9] Fix incorrect frame rate detection of some videos with
 variable frame rate.

Use FFMPEG's r_frame_rate, if it as valid, as a video stream frame rate, otherwise use avg_frame_rate.
Also remove CDVDStreamInfo.rfpsscale, CDVDStreamInfo.rfpsrate, CDemuxStreamVideo.irFpsScale and CDemuxStreamVideo.irFpsRate,
they are not needed anymore.
---
 xbmc/cores/dvdplayer/DVDDemuxers/DVDDemux.h        |  4 ---
 .../cores/dvdplayer/DVDDemuxers/DVDDemuxFFmpeg.cpp | 29 ++++++++--------------
 xbmc/cores/dvdplayer/DVDStreamInfo.cpp             |  8 ------
 xbmc/cores/dvdplayer/DVDStreamInfo.h               |  2 --
 4 files changed, 10 insertions(+), 33 deletions(-)

diff --git a/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemux.h b/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemux.h
index e380056..d93b1dc 100644
--- a/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemux.h
+++ b/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemux.h
@@ -149,8 +149,6 @@ public:
   {
     iFpsScale = 0;
     iFpsRate = 0;
-    irFpsScale = 0;
-    irFpsRate = 0;
     iHeight = 0;
     iWidth = 0;
     fAspect = 0.0;
@@ -165,8 +163,6 @@ public:
   virtual ~CDemuxStreamVideo() {}
   int iFpsScale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
   int iFpsRate;
-  int irFpsScale;
-  int irFpsRate;
   int iHeight; // height of the stream reported by the demuxer
   int iWidth; // width of the stream reported by the demuxer
   float fAspect; // display aspect of stream
diff --git a/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemuxFFmpeg.cpp b/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemuxFFmpeg.cpp
index 88d486b..48fa272 100644
--- a/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemuxFFmpeg.cpp
+++ b/xbmc/cores/dvdplayer/DVDDemuxers/DVDDemuxFFmpeg.cpp
@@ -1151,34 +1151,25 @@ CDemuxStream* CDVDDemuxFFmpeg::AddStream(int iId)
 #else
         AVRational r_frame_rate = pStream->r_frame_rate;
 #endif
+        int rFrameRate = 0;
+        if (r_frame_rate.den && r_frame_rate.num)
+          rFrameRate = r_frame_rate.num / r_frame_rate.den;
+        bool rFrameRateValid = rFrameRate >= 5 && rFrameRate <= 100;
 
-        //average fps is more accurate for mkv files
-        if (m_bMatroska && pStream->avg_frame_rate.den && pStream->avg_frame_rate.num)
-        {
-          st->iFpsRate = pStream->avg_frame_rate.num;
-          st->iFpsScale = pStream->avg_frame_rate.den;
-        }
-        else if(r_frame_rate.den && r_frame_rate.num)
+        if (rFrameRateValid)
         {
           st->iFpsRate = r_frame_rate.num;
           st->iFpsScale = r_frame_rate.den;
         }
-        else
-        {
-          st->iFpsRate  = 0;
-          st->iFpsScale = 0;
-        }
-
-        // added for aml hw decoder, mkv frame-rate can be wrong.
-        if (r_frame_rate.den && r_frame_rate.num)
+        else if(pStream->avg_frame_rate.den && pStream->avg_frame_rate.num)
         {
-          st->irFpsRate = r_frame_rate.num;
-          st->irFpsScale = r_frame_rate.den;
+          st->iFpsRate = pStream->avg_frame_rate.num;
+          st->iFpsScale = pStream->avg_frame_rate.den;
         }
         else
         {
-          st->irFpsRate = 0;
-          st->irFpsScale = 0;
+          st->iFpsRate  = 0;
+          st->iFpsScale = 0;
         }
 
         if (pStream->codec_info_nb_frames >  0
diff --git a/xbmc/cores/dvdplayer/DVDStreamInfo.cpp b/xbmc/cores/dvdplayer/DVDStreamInfo.cpp
index fa0defa..b48cd61 100644
--- a/xbmc/cores/dvdplayer/DVDStreamInfo.cpp
+++ b/xbmc/cores/dvdplayer/DVDStreamInfo.cpp
@@ -51,8 +51,6 @@ void CDVDStreamInfo::Clear()
 
   fpsscale = 0;
   fpsrate  = 0;
-  rfpsscale= 0;
-  rfpsrate = 0;
   height   = 0;
   width    = 0;
   aspect   = 0.0;
@@ -96,8 +94,6 @@ bool CDVDStreamInfo::Equal(const CDVDStreamInfo& right, bool withextradata)
   // VIDEO
   if( fpsscale != right.fpsscale
   ||  fpsrate  != right.fpsrate
-  ||  rfpsscale!= right.rfpsscale
-  ||  rfpsrate != right.rfpsrate
   ||  height   != right.height
   ||  width    != right.width
   ||  stills   != right.stills
@@ -158,8 +154,6 @@ void CDVDStreamInfo::Assign(const CDVDStreamInfo& right, bool withextradata)
   // VIDEO
   fpsscale = right.fpsscale;
   fpsrate  = right.fpsrate;
-  rfpsscale= right.rfpsscale;
-  rfpsrate = right.rfpsrate;
   height   = right.height;
   width    = right.width;
   aspect   = right.aspect;
@@ -219,8 +213,6 @@ void CDVDStreamInfo::Assign(const CDemuxStream& right, bool withextradata)
     const CDemuxStreamVideo *stream = static_cast<const CDemuxStreamVideo*>(&right);
     fpsscale  = stream->iFpsScale;
     fpsrate   = stream->iFpsRate;
-    rfpsscale = stream->irFpsScale;
-    rfpsrate  = stream->irFpsRate;
     height    = stream->iHeight;
     width     = stream->iWidth;
     aspect    = stream->fAspect;
diff --git a/xbmc/cores/dvdplayer/DVDStreamInfo.h b/xbmc/cores/dvdplayer/DVDStreamInfo.h
index c0e22a2..8953ff3 100644
--- a/xbmc/cores/dvdplayer/DVDStreamInfo.h
+++ b/xbmc/cores/dvdplayer/DVDStreamInfo.h
@@ -58,8 +58,6 @@ public:
   // VIDEO
   int fpsscale; // scale of 1001 and a rate of 60000 will result in 59.94 fps
   int fpsrate;
-  int rfpsscale;
-  int rfpsrate;
   int height; // height of the stream reported by the demuxer
   int width; // width of the stream reported by the demuxer
   float aspect; // display aspect as reported by demuxer
-- 
2.1.0


diff --git a/xbmc/cores/AudioEngine/Sinks/AESinkALSA.cpp b/xbmc/cores/AudioEngine/Sinks/AESinkALSA.cpp
index e22db7a..cfbb63d 100644
--- a/xbmc/cores/AudioEngine/Sinks/AESinkALSA.cpp
+++ b/xbmc/cores/AudioEngine/Sinks/AESinkALSA.cpp
@@ -45,6 +45,7 @@
 #define ALSA_CHMAP_KERNEL_BLACKLIST
 
 #define ALSA_OPTIONS (SND_PCM_NO_AUTO_FORMAT | SND_PCM_NO_AUTO_CHANNELS | SND_PCM_NO_AUTO_RESAMPLE)
+#define ALSA_PERIODS 8
 
 #define ALSA_MAX_CHANNELS 16
 static enum AEChannel LegacyALSAChannelMap[ALSA_MAX_CHANNELS + 1] = {
@@ -730,7 +731,19 @@ bool CAESinkALSA::InitializeHW(const ALSAConfig &inconfig, ALSAConfig &outconfig
     }
   }
 
+  unsigned int periods;
   snd_pcm_uframes_t periodSize, bufferSize;
+
+  snd_pcm_hw_params_get_periods_min(hw_params, &periods, NULL);
+  snd_pcm_hw_params_get_period_size_min(hw_params, &periodSize, NULL);
+  snd_pcm_hw_params_get_buffer_size_min(hw_params, &bufferSize);
+  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Min: periodSize %lu, periods %u, bufferSize %lu", periodSize, periods, bufferSize);
+
+  snd_pcm_hw_params_get_periods_max(hw_params, &periods, NULL);
+  snd_pcm_hw_params_get_period_size_max(hw_params, &periodSize, NULL);
+  snd_pcm_hw_params_get_buffer_size_max(hw_params, &bufferSize);
+  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Max: periodSize %lu, periods %u, bufferSize %lu", periodSize, periods, bufferSize);
+
   snd_pcm_hw_params_get_buffer_size_max(hw_params, &bufferSize);
   snd_pcm_hw_params_get_period_size_max(hw_params, &periodSize, NULL);
 
@@ -749,7 +762,11 @@ bool CAESinkALSA::InitializeHW(const ALSAConfig &inconfig, ALSAConfig &outconfig
   */
   periodSize = std::min(periodSize, bufferSize / 4);
 
-  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Request: periodSize %lu, bufferSize %lu", periodSize, bufferSize);
+  bufferSize  = std::min(bufferSize, (snd_pcm_uframes_t)8192);
+  periodSize  = bufferSize / ALSA_PERIODS;
+  periods     = ALSA_PERIODS;
+
+  CLog::Log(LOGDEBUG, "CAESinkALSA::InitializeHW - Request: periodSize %lu, periods %u, bufferSize %lu", periodSize, periods, bufferSize);
 
   snd_pcm_hw_params_t *hw_params_copy;
   snd_pcm_hw_params_alloca(&hw_params_copy);

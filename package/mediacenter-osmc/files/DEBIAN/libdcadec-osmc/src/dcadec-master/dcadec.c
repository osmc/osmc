/*
 * This file is part of libdcadec.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#ifdef _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#endif

#include "libdcadec/dca_stream.h"
#include "libdcadec/dca_context.h"
#include "libdcadec/dca_waveout.h"

static void print_help(char *name)
{
    fprintf(stderr,
"Usage: %s [-26bcfhlmnPqSx] <input.dts> [output.wav]\n"
"dcadec is a free DTS Coherent Acoustics decoder. Supported options:\n"
"\n"
"-2  Extract embedded 2.0 downmix if present, otherwise extract 5.1 downmix.\n"
"\n"
"-6  Extract embedded 5.1 downmix.\n"
"\n"
"-b  Force fixed point DTS core interpolation. Developer option, degrades sound\n"
"    quality.\n"
"\n"
"-c  Force decoding of DTS core only without extensions.\n"
"\n"
"-f  Use FIR filter for floating point DTS core LFE channel interpolation.\n"
"\n"
"-h  Show this help message.\n"
"\n"
"-l  Enable lenient decoding mode. Attempt to recover from errors by skipping\n"
"    non-decodable parts of the stream.\n"
"\n"
"-m  Write a mono WAV file for each native DTS channel. Output file name must\n"
"    include `%%s' sub-string that will be replaced with DTS channel name.\n"
"\n"
"-n  No-act mode. Parse DTS bitstream without writing WAV file(s).\n"
"\n"
"-P  Disable progress indicator.\n"
"\n"
"-q  Be quiet. Disables informational messages and progress indicator. Warnings\n"
"    and errors are still printed.\n"
"\n"
"-S  Don't strip padding samples for streams within DTS-HD container.\n"
"\n"
"-x  Force use of X96 synthesis filter for DTS core interpolation. Developer\n"
"    option, degrades sound quality.\n"
"\n"
"When run without output file name argument, prints information about DTS file\n"
"to stdout and exits.\n"
"\n"
"Single dash in place of input or output file name argument means to read from\n"
"stdin or write to stdout, respectively.\n"
"\n"
"dcadec comes with ABSOLUTELY NO WARRANTY. This is free software, and you are\n"
"welcome to redistribute it under certain conditions; see GNU Lesser General\n"
"Public License version 2.1 for details.\n", name);
}

static const char * const spkr_pair_names[] = {
    "C",    "LR",     "LsRs",   "LFE",
    "Cs",   "LhRh",   "LsrRsr", "Ch",
    "Oh",   "LcRc",   "LwRw",   "LssRss",
    "LFE2", "LhsRhs", "Chr",    "LhrRhr"
};

static char *make_spkr_mask_str(int mask)
{
    static char buf[128];

    if (!mask)
        return "???";

    buf[0] = 0;
    for (int i = 0; i < 16; i++) {
        if (mask & (1 << i)) {
            if (buf[0])
                strcat(buf, " ");
            strcat(buf, spkr_pair_names[i]);
        }
    }

    return buf;
}

static void print_info(struct dcadec_context *context, FILE *fp)
{
    struct dcadec_exss_info *exss = dcadec_context_get_exss_info(context);

    if (exss && exss->profile == DCADEC_PROFILE_DS) {
        dcadec_context_free_exss_info(exss);
        exss = NULL;
    }

    if (exss) {
        if (exss->profile & DCADEC_PROFILE_HD_MA)
            fprintf(fp, "DTS-HD Master Audio");
        else if (exss->profile & DCADEC_PROFILE_HD_HRA)
            fprintf(fp, "DTS-HD High Resolution Audio");
        else if (exss->profile & DCADEC_PROFILE_DS_ES)
            fprintf(fp, "DTS-ES Discrete");
        else if (exss->profile & DCADEC_PROFILE_DS_96_24)
            fprintf(fp, "DTS 96/24");
        else if (exss->profile & DCADEC_PROFILE_EXPRESS)
            fprintf(fp, "DTS Express");
        else
            fprintf(fp, "Unknown Extension Profile");
        fprintf(fp, ": %d ch (%s), %.f kHz, %d bit\n",
            exss->nchannels, make_spkr_mask_str(exss->spkr_mask),
            exss->sample_rate / 1000.0f, exss->bits_per_sample);
        dcadec_context_free_exss_info(exss);
    }

    struct dcadec_core_info *core = dcadec_context_get_core_info(context);
    if (core) {
        if (exss)
            fprintf(fp, "(");
        fprintf(fp, "DTS Core Audio: %d.%d ch, %.f kHz, %d bit",
            core->nchannels, !!core->lfe_present, core->sample_rate / 1000.f,
            core->source_pcm_res);
        if (core->es_format)
            fprintf(fp, ", ES");
        if (core->bit_rate > 0)
            fprintf(fp, ", %.f kbps", core->bit_rate / 1000.0f);
        if (exss)
            fprintf(fp, ")");
        fprintf(fp, "\n");
        dcadec_context_free_core_info(core);
    }
}

static bool interrupted;

#ifdef _WIN32
static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType)
{
    (void)dwCtrlType;
    interrupted = true;
    return TRUE;
}
#else
static void signal_handler(int sig)
{
    (void)sig;
    interrupted = true;
}
#endif

int main(int argc, char **argv)
{
    int flags = DCADEC_FLAG_STRICT;
    int wave_flags = 0;
    bool parse_only = false;
    bool no_progress = false;
    bool quiet = false;
    bool no_strip = false;

    int opt;
    while ((opt = getopt(argc, argv, "26bcfhlmnPqSsx")) != -1) {
        switch (opt) {
        case '2':
            flags |= DCADEC_FLAG_KEEP_DMIX_2CH;
            break;
        case '6':
            flags |= DCADEC_FLAG_KEEP_DMIX_6CH;
            break;
        case 'b':
            flags |= DCADEC_FLAG_CORE_BIT_EXACT;
            break;
        case 'c':
            flags |= DCADEC_FLAG_CORE_ONLY;
            break;
        case 'f':
            flags |= DCADEC_FLAG_CORE_LFE_FIR;
            break;
        case 'h':
            print_help(argv[0]);
            return 0;
        case 'l':
            flags &= ~DCADEC_FLAG_STRICT;
            wave_flags |= DCADEC_WAVEOUT_FLAG_CLIP;
            break;
        case 'm':
            flags |= DCADEC_FLAG_NATIVE_LAYOUT;
            wave_flags |= DCADEC_WAVEOUT_FLAG_MONO;
            break;
        case 'n':
            parse_only = true;
            break;
        case 'P':
            no_progress = true;
            break;
        case 'q':
            quiet = true;
            break;
        case 'S':
            no_strip = true;
            break;
        case 'x':
            flags |= DCADEC_FLAG_CORE_SYNTH_X96;
            break;
        default:
            print_help(argv[0]);
            return 1;
        }
    }

    no_progress |= quiet;

    if (optind >= argc) {
        print_help(argv[0]);
        return 1;
    }

    char *fn = argv[optind];
    struct dcadec_stream *stream = dcadec_stream_open(strcmp(fn, "-") ? fn : NULL, 0);
    if (!stream) {
        fprintf(stderr, "Couldn't open input file\n");
        return 1;
    }

    uint8_t *packet;
    size_t size;
    int ret;

    if ((ret = dcadec_stream_read(stream, &packet, &size)) < 0) {
        fprintf(stderr, "Error reading packet: %s\n", dcadec_strerror(ret));
        dcadec_stream_close(stream);
        return 1;
    }

    if (ret == 0) {
        fprintf(stderr, "This doesn't look like a valid DTS bit stream\n");
        dcadec_stream_close(stream);
        return 1;
    }

    struct dcadec_context *context = dcadec_context_create(flags);
    if (!context) {
        fprintf(stderr, "Couldn't create decoder context\n");
        dcadec_stream_close(stream);
        return 1;
    }

    if ((ret = dcadec_context_parse(context, packet, size)) < 0) {
        fprintf(stderr, "Error parsing packet: %s\n", dcadec_strerror(ret));
        dcadec_context_destroy(context);
        dcadec_stream_close(stream);
        return 1;
    }

    if (!quiet)
        print_info(context, optind + 1 >= argc ? stdout : stderr);

    struct dcadec_waveout *waveout = NULL;
    if (!parse_only) {
        if (optind + 1 >= argc) {
            dcadec_context_destroy(context);
            dcadec_stream_close(stream);
            return 0;
        }

        fn = argv[optind + 1];
        waveout = dcadec_waveout_open(strcmp(fn, "-") ? fn : NULL, wave_flags);
        if (!waveout) {
            fprintf(stderr, "Couldn't open output file\n");
            dcadec_context_destroy(context);
            dcadec_stream_close(stream);
            return 1;
        }

    }

    int last_progress = -1;

#ifdef _WIN32
    SetConsoleCtrlHandler(&console_ctrl_handler, TRUE);
#else
    signal(SIGINT, &signal_handler);
#endif

    uint32_t ndelayframes = 0;
    uint64_t npcmsamples = UINT64_MAX;
    uint64_t nclippedsamples = 0;
    uint64_t nskippedframes = 0;

    if (!parse_only && !no_strip) {
        struct dcadec_stream_info *info = dcadec_stream_get_info(stream);
        if (info) {
            if (info->nframesamples)
                ndelayframes = info->ndelaysamples / info->nframesamples;
            if (info->npcmsamples)
                npcmsamples = info->npcmsamples;
            dcadec_stream_free_info(info);
        }
    }

    if (!quiet) {
        if (waveout) {
            if (flags & DCADEC_FLAG_CORE_ONLY)
                fprintf(stderr, "Decoding (core only)...\n");
            else
                fprintf(stderr, "Decoding...\n");
        } else {
            if (flags & DCADEC_FLAG_CORE_ONLY)
                fprintf(stderr, "Parsing (core only)...\n");
            else
                fprintf(stderr, "Parsing...\n");
        }
    }

    while (!interrupted) {
        if (waveout) {
            int **samples, nsamples, channel_mask, sample_rate, bits_per_sample;
            if ((ret = dcadec_context_filter(context, &samples, &nsamples,
                                             &channel_mask, &sample_rate,
                                             &bits_per_sample, NULL)) < 0) {
                fprintf(stderr, "Error filtering frame: %s\n", dcadec_strerror(ret));
                if (flags & DCADEC_FLAG_STRICT) {
                    break;
                } else {
                    nskippedframes++;
                    goto next_packet;
                }
            }

            if (ndelayframes) {
                ndelayframes--;
                goto next_packet;
            }

            if ((uint64_t)nsamples > npcmsamples)
                nsamples = (int)npcmsamples;

            if ((ret = dcadec_waveout_write(waveout, samples, nsamples,
                                            channel_mask, sample_rate,
                                            bits_per_sample)) < 0) {
                fprintf(stderr, "Error writing WAV file: %s\n", dcadec_strerror(ret));
                if ((flags & DCADEC_FLAG_STRICT) || ret != -DCADEC_EOUTCHG) {
                    break;
                } else {
                    nskippedframes++;
                    goto next_packet;
                }
            }

            npcmsamples -= nsamples;
            nclippedsamples += ret;
        }

next_packet:
        if ((ret = dcadec_stream_read(stream, &packet, &size)) < 0) {
            fprintf(stderr, "Error reading packet: %s\n", dcadec_strerror(ret));
            break;
        }

        if (!no_progress) {
            int progress = dcadec_stream_progress(stream);
            if (progress != last_progress) {
                fprintf(stderr, "Progress: %d%%\r", progress);
                last_progress = progress;
            }
        }

        if (ret == 0)
            break;

        if ((ret = dcadec_context_parse(context, packet, size)) < 0) {
            fprintf(stderr, "Error parsing packet: %s\n", dcadec_strerror(ret));
            if (flags & DCADEC_FLAG_STRICT) {
                break;
            } else {
                nskippedframes++;
                goto next_packet;
            }
        }
    }

    if (!quiet) {
        if (last_progress != -1)
            fprintf(stderr, "\n");
        if (interrupted)
            fprintf(stderr, "Interrupted.\n");
        else if (ret == 0)
            fprintf(stderr, "Completed.\n");
        if (nclippedsamples)
            fprintf(stderr, "*** %" PRIu64 " samples clipped ***\n", nclippedsamples);
        if (nskippedframes)
            fprintf(stderr, "*** %" PRIu64 " frames skipped ***\n", nskippedframes);
    }

    dcadec_waveout_close(waveout);
    dcadec_context_destroy(context);
    dcadec_stream_close(stream);
    return !!ret;
}

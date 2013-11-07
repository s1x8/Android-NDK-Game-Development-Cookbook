/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 */

#include "stdafx.h"
#include "sndfile.h"

#ifndef MODPLUG_FASTSOUNDLIB
//#pragma data_seg(".tables")
#endif

static const BYTE ImpulseTrackerPortaVolCmd[16] =
{
	0x00, 0x01, 0x04, 0x08, 0x10, 0x20, 0x40, 0x60,
	0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// Period table for Protracker octaves 0-5:
static const WORD ProTrackerPeriodTable[6 * 12] =
{
	1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 907,
	856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107, 101, 95, 90, 85, 80, 75, 71, 67, 63, 60, 56,
	53, 50, 47, 45, 42, 40, 37, 35, 33, 31, 30, 28
};


static const WORD ProTrackerTunedPeriods[16 * 12] =
{
	1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 907,
	1700, 1604, 1514, 1430, 1348, 1274, 1202, 1134, 1070, 1010, 954, 900,
	1688, 1592, 1504, 1418, 1340, 1264, 1194, 1126, 1064, 1004, 948, 894,
	1676, 1582, 1492, 1408, 1330, 1256, 1184, 1118, 1056, 996, 940, 888,
	1664, 1570, 1482, 1398, 1320, 1246, 1176, 1110, 1048, 990, 934, 882,
	1652, 1558, 1472, 1388, 1310, 1238, 1168, 1102, 1040, 982, 926, 874,
	1640, 1548, 1460, 1378, 1302, 1228, 1160, 1094, 1032, 974, 920, 868,
	1628, 1536, 1450, 1368, 1292, 1220, 1150, 1086, 1026, 968, 914, 862,
	1814, 1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960,
	1800, 1700, 1604, 1514, 1430, 1350, 1272, 1202, 1134, 1070, 1010, 954,
	1788, 1688, 1592, 1504, 1418, 1340, 1264, 1194, 1126, 1064, 1004, 948,
	1774, 1676, 1582, 1492, 1408, 1330, 1256, 1184, 1118, 1056, 996, 940,
	1762, 1664, 1570, 1482, 1398, 1320, 1246, 1176, 1110, 1048, 988, 934,
	1750, 1652, 1558, 1472, 1388, 1310, 1238, 1168, 1102, 1040, 982, 926,
	1736, 1640, 1548, 1460, 1378, 1302, 1228, 1160, 1094, 1032, 974, 920,
	1724, 1628, 1536, 1450, 1368, 1292, 1220, 1150, 1086, 1026, 968, 914
};


// S3M C-4 periods
static const WORD FreqS3MTable[16] =
{
	1712, 1616, 1524, 1440, 1356, 1280,
	1208, 1140, 1076, 1016, 960, 907,
	0, 0, 0, 0
};


// S3M FineTune frequencies
static const WORD S3MFineTuneTable[16] =
{
	7895, 7941, 7985, 8046, 8107, 8169, 8232, 8280,
	8363, 8413, 8463, 8529, 8581, 8651, 8723, 8757, // 8363*2^((i-8)/(12*8))
};


// Sinus table
static const int16_t ModSinusTable[64] =
{
	0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126,
	127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
	0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126,
	-127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12
};

// Triangle wave table (ramp down)
static const int16_t ModRampDownTable[64] =
{
	0, -4, -8, -12, -16, -20, -24, -28, -32, -36, -40, -44, -48, -52, -56, -60,
	-64, -68, -72, -76, -80, -84, -88, -92, -96, -100, -104, -108, -112, -116, -120, -124,
	127, 123, 119, 115, 111, 107, 103, 99, 95, 91, 87, 83, 79, 75, 71, 67,
	63, 59, 55, 51, 47, 43, 39, 35, 31, 27, 23, 19, 15, 11, 7, 3
};

// Square wave table
static const int16_t ModSquareTable[64] =
{
	127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
	127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
	-127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127,
	-127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127
};

// Random wave table
static const int16_t ModRandomTable[64] =
{
	98, -127, -43, 88, 102, 41, -65, -94, 125, 20, -71, -86, -70, -32, -16, -96,
	17, 72, 107, -5, 116, -69, -62, -40, 10, -61, 65, 109, -18, -38, -13, -76,
	-23, 88, 21, -94, 8, 106, 21, -112, 6, 109, 20, -88, -30, 9, -127, 118,
	42, -34, 89, -4, -51, -72, 21, -29, 112, 123, 84, -101, -92, 98, -54, -95
};


// volume fade tables for Retrig Note:
static const int8_t retrigTable1[16] =
{ 0, 0, 0, 0, 0, 0, 10, 8, 0, 0, 0, 0, 0, 0, 24, 32 };

static const int8_t retrigTable2[16] =
{ 0, -1, -2, -4, -8, -16, 0, 0, 0, 1, 2, 4, 8, 16, 0, 0 };



static const WORD XMPeriodTable[104] =
{
	907, 900, 894, 887, 881, 875, 868, 862, 856, 850, 844, 838, 832, 826, 820, 814,
	808, 802, 796, 791, 785, 779, 774, 768, 762, 757, 752, 746, 741, 736, 730, 725,
	720, 715, 709, 704, 699, 694, 689, 684, 678, 675, 670, 665, 660, 655, 651, 646,
	640, 636, 632, 628, 623, 619, 614, 610, 604, 601, 597, 592, 588, 584, 580, 575,
	570, 567, 563, 559, 555, 551, 547, 543, 538, 535, 532, 528, 524, 520, 516, 513,
	508, 505, 502, 498, 494, 491, 487, 484, 480, 477, 474, 470, 467, 463, 460, 457,
	453, 450, 447, 443, 440, 437, 434, 431
};


static const uint32_t XMLinearTable[768] =
{
	535232, 534749, 534266, 533784, 533303, 532822, 532341, 531861,
	531381, 530902, 530423, 529944, 529466, 528988, 528511, 528034,
	527558, 527082, 526607, 526131, 525657, 525183, 524709, 524236,
	523763, 523290, 522818, 522346, 521875, 521404, 520934, 520464,
	519994, 519525, 519057, 518588, 518121, 517653, 517186, 516720,
	516253, 515788, 515322, 514858, 514393, 513929, 513465, 513002,
	512539, 512077, 511615, 511154, 510692, 510232, 509771, 509312,
	508852, 508393, 507934, 507476, 507018, 506561, 506104, 505647,
	505191, 504735, 504280, 503825, 503371, 502917, 502463, 502010,
	501557, 501104, 500652, 500201, 499749, 499298, 498848, 498398,
	497948, 497499, 497050, 496602, 496154, 495706, 495259, 494812,
	494366, 493920, 493474, 493029, 492585, 492140, 491696, 491253,
	490809, 490367, 489924, 489482, 489041, 488600, 488159, 487718,
	487278, 486839, 486400, 485961, 485522, 485084, 484647, 484210,
	483773, 483336, 482900, 482465, 482029, 481595, 481160, 480726,
	480292, 479859, 479426, 478994, 478562, 478130, 477699, 477268,
	476837, 476407, 475977, 475548, 475119, 474690, 474262, 473834,
	473407, 472979, 472553, 472126, 471701, 471275, 470850, 470425,
	470001, 469577, 469153, 468730, 468307, 467884, 467462, 467041,
	466619, 466198, 465778, 465358, 464938, 464518, 464099, 463681,
	463262, 462844, 462427, 462010, 461593, 461177, 460760, 460345,
	459930, 459515, 459100, 458686, 458272, 457859, 457446, 457033,
	456621, 456209, 455797, 455386, 454975, 454565, 454155, 453745,
	453336, 452927, 452518, 452110, 451702, 451294, 450887, 450481,
	450074, 449668, 449262, 448857, 448452, 448048, 447644, 447240,
	446836, 446433, 446030, 445628, 445226, 444824, 444423, 444022,
	443622, 443221, 442821, 442422, 442023, 441624, 441226, 440828,
	440430, 440033, 439636, 439239, 438843, 438447, 438051, 437656,
	437261, 436867, 436473, 436079, 435686, 435293, 434900, 434508,
	434116, 433724, 433333, 432942, 432551, 432161, 431771, 431382,
	430992, 430604, 430215, 429827, 429439, 429052, 428665, 428278,
	427892, 427506, 427120, 426735, 426350, 425965, 425581, 425197,
	424813, 424430, 424047, 423665, 423283, 422901, 422519, 422138,
	421757, 421377, 420997, 420617, 420237, 419858, 419479, 419101,
	418723, 418345, 417968, 417591, 417214, 416838, 416462, 416086,
	415711, 415336, 414961, 414586, 414212, 413839, 413465, 413092,
	412720, 412347, 411975, 411604, 411232, 410862, 410491, 410121,
	409751, 409381, 409012, 408643, 408274, 407906, 407538, 407170,
	406803, 406436, 406069, 405703, 405337, 404971, 404606, 404241,
	403876, 403512, 403148, 402784, 402421, 402058, 401695, 401333,
	400970, 400609, 400247, 399886, 399525, 399165, 398805, 398445,
	398086, 397727, 397368, 397009, 396651, 396293, 395936, 395579,
	395222, 394865, 394509, 394153, 393798, 393442, 393087, 392733,
	392378, 392024, 391671, 391317, 390964, 390612, 390259, 389907,
	389556, 389204, 388853, 388502, 388152, 387802, 387452, 387102,
	386753, 386404, 386056, 385707, 385359, 385012, 384664, 384317,
	383971, 383624, 383278, 382932, 382587, 382242, 381897, 381552,
	381208, 380864, 380521, 380177, 379834, 379492, 379149, 378807,

	378466, 378124, 377783, 377442, 377102, 376762, 376422, 376082,
	375743, 375404, 375065, 374727, 374389, 374051, 373714, 373377,
	373040, 372703, 372367, 372031, 371695, 371360, 371025, 370690,
	370356, 370022, 369688, 369355, 369021, 368688, 368356, 368023,
	367691, 367360, 367028, 366697, 366366, 366036, 365706, 365376,
	365046, 364717, 364388, 364059, 363731, 363403, 363075, 362747,
	362420, 362093, 361766, 361440, 361114, 360788, 360463, 360137,
	359813, 359488, 359164, 358840, 358516, 358193, 357869, 357547,
	357224, 356902, 356580, 356258, 355937, 355616, 355295, 354974,
	354654, 354334, 354014, 353695, 353376, 353057, 352739, 352420,
	352103, 351785, 351468, 351150, 350834, 350517, 350201, 349885,
	349569, 349254, 348939, 348624, 348310, 347995, 347682, 347368,
	347055, 346741, 346429, 346116, 345804, 345492, 345180, 344869,
	344558, 344247, 343936, 343626, 343316, 343006, 342697, 342388,
	342079, 341770, 341462, 341154, 340846, 340539, 340231, 339924,
	339618, 339311, 339005, 338700, 338394, 338089, 337784, 337479,
	337175, 336870, 336566, 336263, 335959, 335656, 335354, 335051,
	334749, 334447, 334145, 333844, 333542, 333242, 332941, 332641,
	332341, 332041, 331741, 331442, 331143, 330844, 330546, 330247,
	329950, 329652, 329355, 329057, 328761, 328464, 328168, 327872,
	327576, 327280, 326985, 326690, 326395, 326101, 325807, 325513,
	325219, 324926, 324633, 324340, 324047, 323755, 323463, 323171,
	322879, 322588, 322297, 322006, 321716, 321426, 321136, 320846,
	320557, 320267, 319978, 319690, 319401, 319113, 318825, 318538,
	318250, 317963, 317676, 317390, 317103, 316817, 316532, 316246,
	315961, 315676, 315391, 315106, 314822, 314538, 314254, 313971,
	313688, 313405, 313122, 312839, 312557, 312275, 311994, 311712,
	311431, 311150, 310869, 310589, 310309, 310029, 309749, 309470,
	309190, 308911, 308633, 308354, 308076, 307798, 307521, 307243,
	306966, 306689, 306412, 306136, 305860, 305584, 305308, 305033,
	304758, 304483, 304208, 303934, 303659, 303385, 303112, 302838,
	302565, 302292, 302019, 301747, 301475, 301203, 300931, 300660,
	300388, 300117, 299847, 299576, 299306, 299036, 298766, 298497,
	298227, 297958, 297689, 297421, 297153, 296884, 296617, 296349,
	296082, 295815, 295548, 295281, 295015, 294749, 294483, 294217,
	293952, 293686, 293421, 293157, 292892, 292628, 292364, 292100,
	291837, 291574, 291311, 291048, 290785, 290523, 290261, 289999,
	289737, 289476, 289215, 288954, 288693, 288433, 288173, 287913,
	287653, 287393, 287134, 286875, 286616, 286358, 286099, 285841,
	285583, 285326, 285068, 284811, 284554, 284298, 284041, 283785,
	283529, 283273, 283017, 282762, 282507, 282252, 281998, 281743,
	281489, 281235, 280981, 280728, 280475, 280222, 279969, 279716,
	279464, 279212, 278960, 278708, 278457, 278206, 277955, 277704,
	277453, 277203, 276953, 276703, 276453, 276204, 275955, 275706,
	275457, 275209, 274960, 274712, 274465, 274217, 273970, 273722,
	273476, 273229, 272982, 272736, 272490, 272244, 271999, 271753,
	271508, 271263, 271018, 270774, 270530, 270286, 270042, 269798,
	269555, 269312, 269069, 268826, 268583, 268341, 268099, 267857
};


static const int8_t ft2VibratoTable[256] =
{
	0, -2, -3, -5, -6, -8, -9, -11, -12, -14, -16, -17, -19, -20, -22, -23,
	-24, -26, -27, -29, -30, -32, -33, -34, -36, -37, -38, -39, -41, -42,
	-43, -44, -45, -46, -47, -48, -49, -50, -51, -52, -53, -54, -55, -56,
	-56, -57, -58, -59, -59, -60, -60, -61, -61, -62, -62, -62, -63, -63,
	-63, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -63, -63,
	-63, -62, -62, -62, -61, -61, -60, -60, -59, -59, -58, -57, -56, -56,
	-55, -54, -53, -52, -51, -50, -49, -48, -47, -46, -45, -44, -43, -42,
	-41, -39, -38, -37, -36, -34, -33, -32, -30, -29, -27, -26, -24, -23,
	-22, -20, -19, -17, -16, -14, -12, -11, -9, -8, -6, -5, -3, -2, 0,
	2, 3, 5, 6, 8, 9, 11, 12, 14, 16, 17, 19, 20, 22, 23, 24, 26, 27, 29, 30,
	32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
	52, 53, 54, 55, 56, 56, 57, 58, 59, 59, 60, 60, 61, 61, 62, 62, 62, 63,
	63, 63, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62,
	62, 61, 61, 60, 60, 59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49,
	48, 47, 46, 45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27,
	26, 24, 23, 22, 20, 19, 17, 16, 14, 12, 11, 9, 8, 6, 5, 3, 2
};



static const DWORD FineLinearSlideUpTable[16] =
{
	65536, 65595, 65654, 65714,   65773, 65832, 65892, 65951,
	66011, 66071, 66130, 66190, 66250, 66309, 66369, 66429
};


static const DWORD FineLinearSlideDownTable[16] =
{
	65535, 65477, 65418, 65359, 65300, 65241, 65182, 65123,
	65065, 65006, 64947, 64888, 64830, 64772, 64713, 64645
};


static const DWORD LinearSlideUpTable[256] =
{
	65536, 65773, 66010, 66249, 66489, 66729, 66971, 67213,
	67456, 67700, 67945, 68190, 68437, 68685, 68933, 69182,
	69432, 69684, 69936, 70189, 70442, 70697, 70953, 71209,
	71467, 71725, 71985, 72245, 72507, 72769, 73032, 73296,
	73561, 73827, 74094, 74362, 74631, 74901, 75172, 75444,
	75717, 75991, 76265, 76541, 76818, 77096, 77375, 77655,
	77935, 78217, 78500, 78784, 79069, 79355, 79642, 79930,
	80219, 80509, 80800, 81093, 81386, 81680, 81976, 82272,
	82570, 82868, 83168, 83469, 83771, 84074, 84378, 84683,
	84989, 85297, 85605, 85915, 86225, 86537, 86850, 87164,
	87480, 87796, 88113, 88432, 88752, 89073, 89395, 89718,
	90043, 90369, 90695, 91023, 91353, 91683, 92015, 92347,
	92681, 93017, 93353, 93691, 94029, 94370, 94711, 95053,
	95397, 95742, 96088, 96436, 96785, 97135, 97486, 97839,
	98193, 98548, 98904, 99262, 99621, 99981, 100343, 100706,
	101070, 101435, 101802, 102170, 102540, 102911, 103283, 103657,
	104031, 104408, 104785, 105164, 105545, 105926, 106309, 106694,
	107080, 107467, 107856, 108246, 108637, 109030, 109425, 109820,
	110217, 110616, 111016, 111418, 111821, 112225, 112631, 113038,
	113447, 113857, 114269, 114682, 115097, 115514, 115931, 116351,
	116771, 117194, 117618, 118043, 118470, 118898, 119328, 119760,
	120193, 120628, 121064, 121502, 121941, 122382, 122825, 123269,
	123715, 124162, 124611, 125062, 125514, 125968, 126424, 126881,
	127340, 127801, 128263, 128727, 129192, 129660, 130129, 130599,
	131072, 131546, 132021, 132499, 132978, 133459, 133942, 134426,
	134912, 135400, 135890, 136381, 136875, 137370, 137866, 138365,
	138865, 139368, 139872, 140378, 140885, 141395, 141906, 142419,
	142935, 143451, 143970, 144491, 145014, 145538, 146064, 146593,
	147123, 147655, 148189, 148725, 149263, 149803, 150344, 150888,
	151434, 151982, 152531, 153083, 153637, 154192, 154750, 155310,
	155871, 156435, 157001, 157569, 158138, 158710, 159284, 159860,
	160439, 161019, 161601, 162186, 162772, 163361, 163952, 164545,
};


static const DWORD LinearSlideDownTable[256] =
{
	65536, 65299, 65064, 64830, 64596, 64363, 64131, 63900,
	63670, 63440, 63212, 62984, 62757, 62531, 62305, 62081,
	61857, 61634, 61412, 61191, 60970, 60751, 60532, 60314,
	60096, 59880, 59664, 59449, 59235, 59021, 58809, 58597,
	58385, 58175, 57965, 57757, 57548, 57341, 57134, 56928,
	56723, 56519, 56315, 56112, 55910, 55709, 55508, 55308,
	55108, 54910, 54712, 54515, 54318, 54123, 53928, 53733,

	53540, 53347, 53154, 52963, 52772, 52582, 52392, 52204,
	52015, 51828, 51641, 51455, 51270, 51085, 50901, 50717,
	50535, 50353, 50171, 49990, 49810, 49631, 49452, 49274,
	49096, 48919, 48743, 48567, 48392, 48218, 48044, 47871,
	47698, 47526, 47355, 47185, 47014, 46845, 46676, 46508,
	46340, 46173, 46007, 45841, 45676, 45511, 45347, 45184,
	45021, 44859, 44697, 44536, 44376, 44216, 44056, 43898,
	43740, 43582, 43425, 43268, 43112, 42957, 42802, 42648,
	42494, 42341, 42189, 42037, 41885, 41734, 41584, 41434,
	41285, 41136, 40988, 40840, 40693, 40546, 40400, 40254,
	40109, 39965, 39821, 39677, 39534, 39392, 39250, 39108,
	38967, 38827, 38687, 38548, 38409, 38270, 38132, 37995,
	37858, 37722, 37586, 37450, 37315, 37181, 37047, 36913,
	36780, 36648, 36516, 36384, 36253, 36122, 35992, 35862,
	35733, 35604, 35476, 35348, 35221, 35094, 34968, 34842,
	34716, 34591, 34466, 34342, 34218, 34095, 33972, 33850,
	33728, 33606, 33485, 33364, 33244, 33124, 33005, 32886,
	32768, 32649, 32532, 32415, 32298, 32181, 32065, 31950,
	31835, 31720, 31606, 31492, 31378, 31265, 31152, 31040,
	30928, 30817, 30706, 30595, 30485, 30375, 30266, 30157,
	30048, 29940, 29832, 29724, 29617, 29510, 29404, 29298,
	29192, 29087, 28982, 28878, 28774, 28670, 28567, 28464,
	28361, 28259, 28157, 28056, 27955, 27854, 27754, 27654,
	27554, 27455, 27356, 27257, 27159, 27061, 26964, 26866,
	26770, 26673, 26577, 26481, 26386, 26291, 26196, 26102,
};


static const int SpectrumSinusTable[256 * 2] =
{
	0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 7, 8, 9, 10, 10, 11,
	12, 13, 14, 14, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 22, 23,
	24, 25, 25, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 34,
	35, 36, 36, 37, 38, 38, 39, 39, 40, 41, 41, 42, 42, 43, 44, 44,
	45, 45, 46, 46, 47, 47, 48, 48, 49, 49, 50, 50, 51, 51, 52, 52,
	53, 53, 53, 54, 54, 55, 55, 55, 56, 56, 57, 57, 57, 58, 58, 58,
	59, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 61, 62, 62, 62,
	62, 62, 62, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
	63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 62, 62,
	62, 62, 62, 62, 61, 61, 61, 61, 61, 60, 60, 60, 60, 59, 59, 59,
	59, 58, 58, 58, 57, 57, 57, 56, 56, 55, 55, 55, 54, 54, 53, 53,
	53, 52, 52, 51, 51, 50, 50, 49, 49, 48, 48, 47, 47, 46, 46, 45,
	45, 44, 44, 43, 42, 42, 41, 41, 40, 39, 39, 38, 38, 37, 36, 36,
	35, 34, 34, 33, 32, 32, 31, 30, 30, 29, 28, 28, 27, 26, 25, 25,
	24, 23, 22, 22, 21, 20, 20, 19, 18, 17, 17, 16, 15, 14, 14, 13,
	12, 11, 10, 10, 9, 8, 7, 7, 6, 5, 4, 3, 3, 2, 1, 0,
	0, -1, -1, -2, -3, -3, -4, -5, -6, -7, -7, -8, -9, -10, -10, -11,
	-12, -13, -14, -14, -15, -16, -17, -17, -18, -19, -20, -20, -21, -22, -22, -23,
	-24, -25, -25, -26, -27, -28, -28, -29, -30, -30, -31, -32, -32, -33, -34, -34,
	-35, -36, -36, -37, -38, -38, -39, -39, -40, -41, -41, -42, -42, -43, -44, -44,
	-45, -45, -46, -46, -47, -47, -48, -48, -49, -49, -50, -50, -51, -51, -52, -52,
	-53, -53, -53, -54, -54, -55, -55, -55, -56, -56, -57, -57, -57, -58, -58, -58,
	-59, -59, -59, -59, -60, -60, -60, -60, -61, -61, -61, -61, -61, -62, -62, -62,
	-62, -62, -62, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63,
	-63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -63, -62, -62,
	-62, -62, -62, -62, -61, -61, -61, -61, -61, -60, -60, -60, -60, -59, -59, -59,
	-59, -58, -58, -58, -57, -57, -57, -56, -56, -55, -55, -55, -54, -54, -53, -53,
	-53, -52, -52, -51, -51, -50, -50, -49, -49, -48, -48, -47, -47, -46, -46, -45,
	-45, -44, -44, -43, -42, -42, -41, -41, -40, -39, -39, -38, -38, -37, -36, -36,
	-35, -34, -34, -33, -32, -32, -31, -30, -30, -29, -28, -28, -27, -26, -25, -25,
	-24, -23, -22, -22, -21, -20, -20, -19, -18, -17, -17, -16, -15, -14, -14, -13,
	-12, -11, -10, -10, -9, -8, -7, -7, -6, -5, -4, -3, -3, -2, -1, 0,
};

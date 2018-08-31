#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

static struct locale {
    unsigned int code;
    const char name[8];
} locales[] = {
    {0x0004, "zh_CHS"},	/* Chinese - Simplified */
    {0x0401, "ar_SA"},	/* Arabic - Saudi Arabia */
    {0x0402, "bg_BG"},	/* Bulgarian */
    {0x0403, "ca_ES"},	/* Catalan */
    {0x0404, "zh_TW"},	/* Chinese - Taiwan */
    {0x0405, "cs_CZ"},	/* Czech */
    {0x0406, "da_DK"},	/* Danish */
    {0x0407, "de_DE"},	/* German - Germany */
    {0x0408, "el_GR"},	/* Greek */
    {0x0409, "en_US"},	/* English - United States */
    {0x040a, "es_ES"},	/* Spanish - Spain */
    {0x040b, "fi_FI"},	/* Finnish */
    {0x040c, "fr_FR"},	/* French - France */
    {0x040d, "he_IL"},	/* Hebrew */
    {0x040e, "hu_HU"},	/* Hungarian */
    {0x040f, "is_IS"},	/* Icelandic */
    {0x0410, "it_IT"},	/* Italian - Italy */
    {0x0411, "ja_JP"},	/* Japanese */
    {0x0412, "ko_KR"},	/* Korean */
    {0x0413, "nl_NL"},	/* Dutch - The Netherlands */
    {0x0414, "nb_NO"},	/* Norwegian - Bokmal */
    {0x0415, "pl_PL"},	/* Polish */
    {0x0416, "pt_BR"},	/* Portuguese - Brazil */
    {0x0417, "rm_CH"},	/* Raeto-Romanese */
    {0x0418, "ro_RO"},	/* Romanian - Romania */
    {0x0419, "ru_RU"},	/* Russian */
    {0x041a, "hr_HR"},	/* Croatian */
    {0x041b, "sk_SK"},	/* Slovak */
    {0x041c, "sq_AL"},	/* Albanian */
    {0x041d, "sv_SE"},	/* Swedish - Sweden */
    {0x041e, "th_TH"},	/* Thai */
    {0x041f, "tr_TR"},	/* Turkish */
    {0x0420, "ur_PK"},	/* Urdu */
    {0x0421, "id_ID"},	/* Indonesian */
    {0x0422, "uk_UA"},	/* Ukrainian */
    {0x0423, "be_BY"},	/* Belarusian */
    {0x0424, "sl_SI"},	/* Slovenian */
    {0x0425, "et_EE"},	/* Estonian */
    {0x0426, "lv_LV"},	/* Latvian */
    {0x0427, "lt_LT"},	/* Lithuanian */
    {0x0429, "fa_IR"},	/* Persian */
    {0x042a, "vi_VN"},	/* Vietnamese */
    {0x042b, "hy_AM"},	/* Armenian */
    {0x042c, "az_AZ"},	/* Azeri Latin */
    {0x042d, "eu_ES"},	/* Basque */
    {0x042f, "mk_MK"},	/* FYRO Macedonian */
    {0x0432, "tn_ZA"},	/* Setswana - Southern Africa */
    {0x0434, "xh_ZA"},	/* Xhosa - South Africa */
    {0x0435, "zu_ZA"},	/* Zulu */
    {0x0436, "af_ZA"},	/* Afrikaans */
    {0x0437, "ka_GE"},	/* Georgian */
    {0x0438, "fo_FO"},	/* Faroese */
    {0x0439, "hi_IN"},	/* Hindi */
    {0x043a, "mt_MT"},	/* Maltese */
    {0x043b, "se_NO"},	/* Sami Northern Norway */
    {0x043e, "ms_MY"},	/* Malay - Malaysia */
    {0x043f, "kk_KZ"},	/* Kazakh */
    {0x0440, "ky_KG"},	/* Kyrgyz */
    {0x0441, "sw_KE"},	/* Swahili */
    {0x0443, "uz_UZ"},	/* Uzbek - Latin */
    {0x0444, "tt_RU"},	/* Tatar */
    {0x0445, "bn_IN"},	/* Begali */
    {0x0446, "pa_IN"},	/* Punjabi */
    {0x0447, "gu_IN"},	/* Gujarati */
    {0x0448, "or_IN"},	/* Oriya - India */
    {0x0449, "ta_IN"},	/* Tamil */
    {0x044a, "te_IN"},	/* Telugu */
    {0x044b, "kn_IN"},	/* Kannada - India */
    {0x044c, "ml_IN"},	/* Malayalam - India */
    {0x044e, "mr_IN"},	/* Marathi */
    {0x044f, "sa_IN"},	/* Sanskrit */
    {0x0450, "mn_MN"},	/* Mongolian */
    {0x0452, "cy_GB"},	/* Welsh */
    {0x0456, "gl_ES"},	/* Galician */
    {0x0457, "kok_IN"},	/* Konkani */
    {0x045a, "syr_SY"},	/* Syriac */
    {0x045d, "iu_CA"},	/* Inuktitut */
    {0x0461, "ne_NP"},	/* Nepali */
    {0x0462, "fy_NL"},	/* Frisian - Netherlands */
    {0x0463, "ps_AF"},	/* Pashto - Afghanistan */
    {0x0464, "fil_PH"},	/* Filipino */
    {0x0465, "div_MV"},	/* Divehi - Maldives */
    {0x046b, "quz_BO"},	/* Quechua (Bolivia) */
    {0x046c, "ns_ZA"},	/* Northern Sotho */
    {0x046e, "lb_LU"},	/* Luxembourgish */
    {0x047a, "arn_CL"},	/* Mapudungun */
    {0x047c, "moh_CA"},	/* Mohawk - Canada */
    {0x047e, "br_FR"},	/* Breton - France */
    {0x0481, "mi_NZ"},	/* Maori */
    {0x0482, "oc_FR"},	/* Occitan - France */
    {0x048c, "gbz_AF"},	/* Dari - Afghanistan */
    {0x0801, "ar_IQ"},	/* Arabic - Iraq */
    {0x0804, "zh_CN"},	/* Chinese - PRC */
    {0x0807, "de_CH"},	/* German - Switzerland */
    {0x0809, "en_GB"},	/* English - United Kingdom */
    {0x080a, "es_MX"},	/* Spanish - Mexico */
    {0x080c, "fr_BE"},	/* French - Belgium */
    {0x0810, "it_CH"},	/* Italian - Switzerland */
    {0x0813, "nl_BE"},	/* Dutch - Belgium */
    {0x0814, "nn_NO"},	/* Norwegian - Nynorsk */
    {0x0816, "pt_PT"},	/* Portuguese - Portugal */
    {0x081a, "sr_SP"},	/* Serbian - Latin */
    {0x081d, "sv_FI"},	/* Swedish - Finland */
    {0x0820, "ur_IN"},	/* Urdu - India */
    {0x082c, "az_AZ"},	/* Azeri - Cyrillic */
    {0x083b, "se_SE"},	/* Sami Northern Sweden */
    {0x083c, "ga_IE"},	/* Irish - Ireland */
    {0x083e, "ms_BN"},	/* Malay - Brunei */
    {0x0843, "uz_UZ"},	/* Uzbek - Cyrillic */
    {0x085d, "iu_CA"},	/* Inuktitut - Latin */
    {0x086b, "quz_EC"},	/* Quechua (Ecuador) */
    {0x0c01, "ar_EG"},	/* Arabic - Egypt */
    {0x0c04, "zh_HK"},	/* Chinese - Hong Kong S.A.R. */
    {0x0c07, "de_AT"},	/* German - Austria */
    {0x0c09, "en_AU"},	/* English - Australia */
    {0x0c0a, "es_ES"},	/* Spanish - Spain (Modern) */
    {0x0c0c, "fr_CA"},	/* French - Canada */
    {0x0c1a, "sr_SP"},	/* Serbian - Cyrillic */
    {0x0c3b, "se_FI"},	/* Sami Northern Finland */
    {0x0c6b, "quz_PE"},	/* Quechua (Peru) */
    {0x1001, "ar_LY"},	/* Arabic - Libya */
    {0x1004, "zh_SG"},	/* Chinese - Singapore */
    {0x1007, "de_LU"},	/* German - Luxembourg */
    {0x1009, "en_CA"},	/* English - Canada */
    {0x100a, "es_GT"},	/* Spanish - Guatemala */
    {0x100c, "fr_CH"},	/* French - Switzerland */
    {0x101a, "hr_BA"},	/* Croatian - Bosnia */
    {0x103b, "smj_NO"},	/* Sami Norway */
    {0x1401, "ar_DZ"},	/* Arabic - Algeria */
    {0x1404, "zh_MO"},	/* Chinese - Macao S.A.R. */
    {0x1407, "de_LI"},	/* German - Liechtenstein */
    {0x1409, "en_NZ"},	/* English - New Zealand */
    {0x140a, "es_CR"},	/* Spanish - Costa Rica */
    {0x140c, "fr_LU"},	/* French - Luxembourg */
    {0x141a, "bs_BA"},	/* Bosnian - Cyrillic */
    {0x143b, "smj_SE"},	/* Sami Sweden */
    {0x1801, "ar_MA"},	/* Arabic - Morocco */
    {0x1809, "en_IE"},	/* English - Ireland */
    {0x180a, "es_PA"},	/* Spanish - Panama */
    {0x180c, "fr_MC"},	/* French - Monaco */
    {0x181a, "sr_BA"},	/* Serbian - Bosnia Latin */
    {0x183b, "sma_NO"},	/* Sami Southern Norway */
    {0x1c01, "ar_TN"},	/* Arabic - Tunisia */
    {0x1c09, "en_ZA"},	/* English - South Africa */
    {0x1c0a, "es_DO"},	/* Spanish - Dominican Republic */
    {0x1c1a, "sr_BA"},	/* Serbian - Bosnia Cyrillic */
    {0x1c3b, "sma_SE"},	/* Sami Southern Sweden */
    {0x2001, "ar_OM"},	/* Arabic - Oman */
    {0x2009, "en_JA"},	/* English - Jamaica */
    {0x200a, "es_VE"},	/* Spanish - Venezuela */
    {0x201a, "bs_BA"},	/* Bosnian */
    {0x203b, "sms_FI"},	/* Sami Skolt */
    {0x2401, "ar_YE"},	/* Arabic - Yemen */
    {0x2409, "en_CB"},	/* English - Carribbean */
    {0x240a, "es_CO"},	/* Spanish - Colombia */
    {0x243b, "smn_FI"},	/* Sami Finland */
    {0x2801, "ar_SY"},	/* Arabic - Syria */
    {0x2809, "en_BZ"},	/* English - Belize */
    {0x280a, "es_PE"},	/* Spanish - Peru */
    {0x2c01, "ar_JO"},	/* Arabic - Jordan */
    {0x2c09, "en_TT"},	/* English - Trinidad */
    {0x2c0a, "es_AR"},	/* Spanish - Argentina */
    {0x3001, "ar_LB"},	/* Arabic - Lebanon */
    {0x3009, "en_ZW"},	/* English - Zimbabwe */
    {0x300a, "es_EC"},	/* Spanish - Ecuador */
    {0x3401, "ar_KW"},	/* Arabic - Kuwait */
    {0x3409, "en_PH"},	/* English - Phillippines */
    {0x340a, "es_CL"},	/* Spanish - Chile */
    {0x3801, "ar_AE"},	/* Arabic - United Arab Emirates */
    {0x380a, "es_UR"},	/* Spanish - Uruguay */
    {0x3c01, "ar_BH"},	/* Arabic - Bahrain */
    {0x3c0a, "es_PY"},	/* Spanish - Paraguay */
    {0x4001, "ar_QA"},	/* Arabic - Qatar */
    {0x400a, "es_BO"},	/* Spanish - Bolivia */
    {0x440a, "es_SV"},	/* Spanish - El Salvador */
    {0x480a, "es_HN"},	/* Spanish - Honduras */
    {0x4c0a, "es_NI"},	/* Spanish - Nicaragua */
    {0x500a, "es_PR"},	/* Spanish - Puerto Rico */
    {0x7c04, "zh_CHT"},	/* Chinese - Traditional */
};

static int num_locales = sizeof(locales) / sizeof(locales[0]);

static int compare(const void *pkey, const void *pval)
{
    LCID code = *(const LCID *)pkey;
    const struct locale *loc = pval;
    return code - loc->code;
}

int main(void)
{
    LCID code = GetUserDefaultLCID();
    struct locale *loc = bsearch(&code,
				 locales, num_locales, sizeof(locales[0]),
				 compare);
    if (!loc)
	return 1;
    fprintf(stdout, "%s\n", loc->name);
    return 0;
}

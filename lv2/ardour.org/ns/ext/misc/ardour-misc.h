
/**
   @defgroup plugin provided bypass

	 A port with the designation "processing#enable" must
	 control a plugin's internal bypass mode.

	 If the port value is larger than zero the plugin processes
	 normally.

	 If the port value is zero, the plugin is expected to bypass
	 all signals unmodified.

	 The plugin is responsible for providing a click-free transition
	 between the states.

	 (values less than zero are reserved for future use:
	 e.g click-free insert/removal of latent plugins.
	 Generally values <= 0 are to be treated as bypassed.)

   lv2:designation <http://ardour.org/lv2/processing#enable> ;

   @{
*/

#define LV2_PROCESSING_URI "http://ardour.org/lv2/processing"
#define LV2_PROCESSING_URI_PREFIX LV2_PROCESSING_URI "#"
#define LV2_PROCESSING_URI__enable LV2_PROCESSING_URI_PREFIX "enable"

/**
   @}
*/

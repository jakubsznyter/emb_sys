#include "xds_cnn.h"

XDs_cnn_Config XDs_cnn_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,ds-cnn-1.0", /* compatible */
		0x40010000 /* reg */
	},
	 {
		 NULL
	}
};
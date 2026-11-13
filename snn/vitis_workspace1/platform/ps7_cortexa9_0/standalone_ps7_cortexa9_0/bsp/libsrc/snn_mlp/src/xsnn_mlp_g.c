#include "xsnn_mlp.h"

XSnn_mlp_Config XSnn_mlp_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,snn-mlp-1.0", /* compatible */
		0x40020000 /* reg */
	},
	 {
		 NULL
	}
};
/**
 * struct build_info
 */
struct build_info {
	char build_version[25];
	char build_time[25];
};

/**
 * struct acpm_framework - General information for ACPM framework.
 *
 * @plugins:		Pointer to soc-specific plugins array.
 * @pid_framework:	Plugin ID for ACPM framework.
 * @pid_max:		# of plugins.
 * @ipc_max:		# of IPC channels.
 * @irq_max:		# of interrupt sources.
 * @ktime_index:	ktime information from the kernel.
 * @log_buf_rear:	Rear pointer of the log buffer.
 * @log_buf_front:	Front pointer of the log buffer.
 * @log_data:		Base address of the log buffer.
 * @log_entry_size:	Entry size of the log buffer.
 * @log_entry_len:	Length of the log buffer.
 * @ipc_base:		Base address of the IPC buffer.
 * @ipc_buf_tx_size:	Size of the IPC TX buffer.
 * @total_size:		Size of the framework and all plugins attached.
 * @fw_size:		Size of the ACPM framework.
 * @intr_to_skip:	Disabled interrupts.
 * @intr_flag_offset:	Field offset for Mailbox interrupt pending register.
 */
// struct acpm_framework {
// 	u64 plugins;
// 	u32 num_plugins;
//         u64 ipc_channels;
//         u32 num_ipc_channels;
// 	u32 pid_framework;
// 	u32 pid_max;
// 	u32 ipc_ap_max;
// 	u32 ipc_cp_max;
// 	u32 ipc_gnss_max;
// 	u32 ipc_wlbt_max;
// 	u32 ipc_max;
// 	u32 irq_max;
// 	u32 ktime_index;
// 	u32 log_buf_rear;
// 	u32 log_buf_front;
// 	u32 log_data;
// 	u32 log_entry_size;
// 	u32 log_entry_len;
// 	u32 ipc_base;
// 	u32 ipc_buf_tx_size;
// 	u32 total_size;
// 	u32 fw_size;
// 	u32 intr_to_skip;
// 	u32 intr_flag_offset;
// 	struct build_info info;
// };

struct acpm_framework {
	u32 plugins;
	u32 num_plugins;
	u32 ipc_channels;
	u32 num_ipc_channels;
	u32 pid_framework;
	u32 pid_max;
	u32 ipc_ap_max;
	u32 ipc_cp_max;
	u32 ipc_gnss_max;
	u32 ipc_wlbt_max;
	u32 ipc_max;
	u32 irq_max;
	u32 ktime_index;
	u32 log_buf_rear;
	u32 log_buf_front;
	u32 log_data;
	u32 log_entry_size;
	u32 log_entry_len;
	u32 ipc_base;
	u32 ipc_buf_tx_size;
	u32 total_size;
	u32 fw_size;
	u32 intr_to_skip;
	u32 intr_flag_offset;
	struct build_info info;
};

/**
 * struct plugin_ops - plugin callbacks to be provided to framework
 *
 * @ipc_handler:	handler to be executed when ipc for this plugin is arrived.
 * @irq_handler:	handler to be executed when hw irq for this plugin is arrived.
 * @timer_event_handler:handler to be executed when requested timer is expired.
 */
struct plugin_ops {
	s32 (*ipc_handler) (struct ipc_cmd_info *cmd, u32 ch_num);
	s32 (*irq_handler) (u32 intr);
	s32 (*timer_event_handler) (void);
	s32 (*extern_func) (u32 *arg0, u32 *arg1, u32 *arg2);
	struct build_info info;
};

/**
 * struct plugin - The basic plugin structure
 *
 * @id:			Predefined id for this plugin.
 * @base_addr:		Predefined base addr for this plugin. (entrypoint)
 * @acpm_ops:		Framework callbacks.
 * @plugin_ops:		Plugin callbacks.
 * @timer:		Timer descriptor for this plugin.
 * @is_attached:	For dynamic plugin support.
 * @size:		The size of this plugin.
 */
struct plugin {
	u32 id;
	void *base_addr;
	u64 acpm_ops;
	struct plugin_ops *plugin_ops;
	u32 secure_func_mask;
        u32 extern_func_mask;
	u64 timer;
	u8 is_attached;
	u32 size;
	u8 stay_attached;
	const char *fw_name;
};

/**
 * struct channel_info - IPC information of a channel
 *
 * @rx_rear:		Rear ptr for RX cmd queue. (for dequeue)
 * @rx_front:		Front ptr for RX cmd queue. (just for empty check)
 * @rx_base:		Predefined base addr for RX cmd queue.
 * @rx_indr_buf:	Predefined base addr for RX indirect buffer.
 * @tx_rear:		Rear ptr for TX queue.
 * @tx_front:		Front ptr for TX queue.
 * @tx_base:		Predefined base addr for TX queue.
 * @q_len:		Length of both TX/RX queue.
 * @q_elem_size:	Element size of both TX/RX queue.
 * @credit:		For preventing starvation of certain plugin.
 */
struct channel_info {
	u32 rx_rear;
	u32 rx_front;
	u32 rx_base;
	u32 rx_indr_buf;
	u32 rx_indr_buf_base;
	u32 rx_indr_buf_size;
	u32 tx_rear;
	u32 tx_front;
	u32 tx_base;
	u32 q_len;
	u32 q_elem_size;
	u32 credit;
	u32 mbox_addr;
};

/**
 * struct ipc_channel - descriptor for ipc channel.
 *
 * @id:			The ipc channel's id.
 * @field:		The ipc channel's field in mailbox status register.
 * @owner:		This interrupt's Belonged plugin.
 * @type:		The ipc channel's memory type; QUEUE or Register.
 * @ch:			IPC information for this plugin's channel.
 * @ap_poll:		The flag indicating whether AP polls on this channel or not. (interrupt)
 */
struct ipc_channel {
	u32 id;
	u32 field;
	s32 owner;
	u32 type;
	struct channel_info ch;
	u32 ap_poll;
};

struct acpm_debug_info {
	unsigned int period;
	void __iomem *time_index;
	unsigned int num_timestamps;
	unsigned long long *timestamps;

	void __iomem *log_buff_rear;
	void __iomem *log_buff_front;
	void __iomem *log_buff_base;
	unsigned int log_buff_len;
	unsigned int log_buff_size;
	void __iomem *dump_base;
	unsigned int dump_size;
	void __iomem *dump_dram_base;
	unsigned int debug_log_level;
	struct delayed_work periodic_work;
	struct work_struct update_log_work;

	spinlock_t lock;
};
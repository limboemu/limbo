#ifndef __ESP_SCSI_H
#define __ESP_SCSI_H

struct disk_op_s;
int esp_scsi_process_op(struct disk_op_s *op);
void esp_scsi_setup(void);

#endif /* __ESP_SCSI_H */

#ifndef NAN_CMD_H_
#define NAN_CMD_H_

#include <state.h>

void nan_handle_cmd(struct nan_state *state, char *input, char **last_cmd);

#endif // NAN_CMD_H
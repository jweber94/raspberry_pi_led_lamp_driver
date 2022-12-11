import logging

class MovingAvgRingbuffer:
    def __init__(self, capacity, heating_clip):
        # See https://towardsdatascience.com/circular-queue-or-ring-buffer-92c7b0193326 for inspiration to the used ringbuffer-like moving avg calculation
        self.queue = [None] * capacity # init with empty array
        self.head = 0
        self.tail = 0

        self.capacity = capacity
        self.currently_used = 0

        self.moving_avg = 0
        self.max_heating_delta = heating_clip # degree
        logging.debug("heating clip for moving avg is " + str(heating_clip))
        logging.info("Moving average ringbuffer initialized with capacity = " + str(self.capacity))

    def get_capacity(self):
        return self.capacity

    def enqueue(self, val):
        # enqueue for FIFO ringbuffer
        # add new value to the queue
        first_element = False
        if self.currently_used == self.capacity: # debug
            logging.debug("Delete old value")
        if all(val is None for val in self.queue):
            self.queue[self.head] = val
            self.moving_avg = val
            first_element = True
        else:
            self.head = (self.head + 1) % self.capacity # setting new tail
            self.queue[self.head] = val
        
        # readjust the head pointer
        if (self.head == self.tail) and (not first_element): # will be always the case after the ringbuffer is completly filled up for the first time
            self.tail= (self.tail + 1) % self.capacity
        else:
            self.currently_used += 1

        # update the average based on the newly inserted value
        if all(val is not None for val in self.queue):
            self.check_heating_finshed()
        self.calc_moving_avg(self.queue[self.head], self.queue[self.tail])

        logging.debug("Currently used = " + str(self.currently_used))
        logging.debug("Added " + str(val) + " to the ringbuffer")
    
    def calc_moving_avg(self, x_t, x_t_n):
        # See https://de.wikipedia.org/wiki/Gleitender_Mittelwert
        assert(self.currently_used <= self.capacity)
        if self.currently_used == 0: # avoid division by 0
            self.moving_avg = 0
        else:
            self.moving_avg = self.moving_avg + x_t/self.currently_used - x_t_n/self.currently_used
        logging.debug("Current average is " + str(self.moving_avg))

    def get_moving_avg(self):
        return self.moving_avg

    def clear_buffer(self):
        self.queue = [None] * self.capacity
        self.currently_used = 0
        self.moving_avg = 0
        self.head = 0
        self.tail = 0

    def print_saved_values(self):
        logging.debug("Elements in moving avg calculation:")
        for elem in self.queue:
            logging.debug(elem)

    def check_heating_finshed(self):
        # average clipping heuristic
        do_clipping = False
        current_val = self.queue[self.head]
        for val in self.queue:
            delta = val - current_val
            if abs(delta) > self.max_heating_delta:
                do_clipping = True
        if do_clipping:
            self.moving_avg = min(self.queue)
            logging.warning("Updated moving average to " + str(self.moving_avg) + " due to too high difference to the current value")

import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Class that implements the channel used by headquarters and space explorers to
 * communicate.
 */
public class CommunicationChannel {

  /*
   * Declare the two channels for the communication between Space Explorers
   * and HeadQuarters. Use LinkedBlockingQueue for solving concurrency problems:
   * * put  -> waiting if necessary for space to become available.
   * * take -> waiting if necessary until an element becomes available.
   *
   * To have the correct order of messages from the Federation HQ
   * * discovered solar system
   * * adjacent undiscovered solar system
   * use a ReentrantLock that will be held for so long to write the two
   * messages on the channel (so for 2 times -> 2 unlocks).
   */
  private final ReentrantLock headQuartersChannelLock;

  /* Where space explorers write to and headquarters read from. */
  private final LinkedBlockingQueue<Message> spaceExplorersChannel;

  /* Where headquarters write to and space explorers read from. */
  private final LinkedBlockingQueue<Message> headQuartersChannel;

  /**
   * Creates a {@code CommunicationChannel} object.
   */
  public CommunicationChannel() {
    headQuartersChannelLock = new ReentrantLock(true);
    spaceExplorersChannel = new LinkedBlockingQueue<>();
    headQuartersChannel = new LinkedBlockingQueue<>();
  }

  /**
   * Puts a message on the space explorer channel (i.e., where space explorers
   * write to and headquarters read from).
   *
   * @param message message to be put on the channel
   */
  public void putMessageSpaceExplorerChannel(Message message) {
    try {
      spaceExplorersChannel.put(message);
    } catch (InterruptedException exception) {
    }
  }

  /**
   * Gets a message from the space explorer channel (i.e., where space explorers
   * write to and headquarters read from).
   *
   * @return message from the space explorer channel
   */
  public Message getMessageSpaceExplorerChannel() {
    try {
      return spaceExplorersChannel.take();
    } catch (InterruptedException exception) {
    }

    return null;
  }

  /**
   * Puts a message on the headquarters channel (i.e., where headquarters write
   * to and space explorers read from).
   *
   * @param message message to be put on the channel
   */
  public void putMessageHeadQuarterChannel(Message message) {
    /*
     * Continue until the data of the message is an END from HeadQuarter.
     *
     * Block the channel until the HeadQuarter writes the two solar systems.
     * Release the channel when the hold counter is equal with 2, so there
     * will be 2 unlocks.
     */
    if (message.getData().compareTo(HeadQuarter.END) != 0) {
      headQuartersChannelLock.lock();

      try {
        headQuartersChannel.put(message);
      } catch (InterruptedException exception) {
      }

      if (headQuartersChannelLock.getHoldCount() == 2) {
        headQuartersChannelLock.unlock();
        headQuartersChannelLock.unlock();
      }
    }
  }

  /**
   * Gets a message from the headquarters channel (i.e., where headquarters
   * write to and space explorer read from).
   *
   * @return message from the header quarter channel
   */
  public Message getMessageHeadQuarterChannel() {
    Message messageOne = null;
    Message messageTwo = null;

    /*
     * Block the channel until the Space Explorer will get:
     * * the parent solar system.
     * * the current solar system.
     */
    synchronized (headQuartersChannel) {
      try {
        messageOne = headQuartersChannel.take();
        messageTwo = headQuartersChannel.take();
      } catch (InterruptedException exception) {
      }
    }

    if (messageOne != null && messageTwo != null) {
      return new Message(
          messageOne.getCurrentSolarSystem(),
          messageTwo.getCurrentSolarSystem(),
          messageTwo.getData()
      );
    }

    return null;
  }
}
